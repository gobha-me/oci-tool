#include <OCI/Copy.hpp>
#include <future>
#include <spdlog/spdlog.h>
#include <vector>

OCI::Copy::Copy()
    : stm_( new gobha::SimpleThreadManager ), progress_bars_( new ProgressBars ),
      working_digests_( new WorkingDigests ) {}
OCI::Copy::Copy( Base::Client *source, Base::Client *destination ) : Copy() {
  src_  = source;
  dest_ = destination;
}
OCI::Copy::Copy( Base::Client *source, Base::Client *destination, STM_ptr stm, PB_ptr progress_bars )
    :                                                                                       // NOLINT
      stm_( stm ), progress_bars_( progress_bars ), src_( source ), dest_( destination ) {} // NOLINT

OCI::Copy::~Copy() = default;

void OCI::Copy::execute( std::string const rsrc, std::string const target ) {
  Schema2::ManifestList ml_request;

  ml_request.name            = rsrc;
  ml_request.requestedTarget = target;

  try {
    auto manifest_list = Manifest< Schema2::ManifestList >( src_->copy().get(), ml_request );

    switch ( manifest_list.schemaVersion ) {
    case 1: // Fall back to Schema1
    {
      Schema1::ImageManifest im_request;

      im_request.name            = rsrc;
      im_request.requestedTarget = target;

      auto image_manifest = Manifest< Schema1::ImageManifest >( src_->copy().get(), im_request );

      if ( not image_manifest.fsLayers.empty() ) {
        spdlog::trace( "OCI::Copy Start Schema1 ImageManifest {}:{}", rsrc, target );
        execute( image_manifest );
        spdlog::trace( "OCI::Copy Finish Schema1 ImageManifest {}:{}", rsrc, target );
      }
    }

    break;
    case 2:
      if ( not manifest_list.manifests.empty() ) {
        spdlog::trace( "OCI::Copy Start Schema2 ManifestList {}:{}", rsrc, target );
        execute( manifest_list );
        spdlog::trace( "OCI::Copy Finish Schema2 ManifestList {}:{}", rsrc, target );
      }

      break;
    default:
      spdlog::error( "Unknown schemaVersion {}", manifest_list.schemaVersion );
    }
  } catch ( std::runtime_error &e ) {
    spdlog::error( "OCI::Copy::execute {}", e.what() );
  }
}

void OCI::Copy::execute( const Schema1::ImageManifest &image_manifest ) {
  for ( auto const &layer : image_manifest.fsLayers ) {
    if ( layer.first == "blobSum" ) {
      if ( not dest_->copy()->hasBlob( image_manifest, layer.second ) ) {
        spdlog::warn( "OCI::Copy::execute Schema1::ImageManifest Destination doesn't have layer '{}'", layer.second );
        spdlog::error( "OCI::Copy::execute Schema1::ImageManifest not implemented" );
      }
    }
  }

  spdlog::warn( "Test is successful and Post Schema1::ImageManifest to OCI::Base::Client::putManifest" );
}

void OCI::Copy::execute( const Schema1::SignedImageManifest &image_manifest ) {
  for ( auto const &layer : image_manifest.fsLayers ) {
    if ( layer.first == "blobSum" ) {
      if ( not dest_->copy()->hasBlob( image_manifest, layer.second ) ) {
        spdlog::warn( "OCI::Copy::execute Schema1::SignedImageManifest Destination doesn't have layer '{}'",
                      layer.second );
        spdlog::error( "OCI::Copy::execute Schema1::SignedImageManifest not implemented" );
      }
    }
  }

  spdlog::warn( "Test is successful and Post Schema1::SignedImageManifest to OCI::Base::Client::putManifest" );
}

void OCI::Copy::execute( Schema2::ManifestList &manifest_list ) {
  try {
    auto                  dest_manifest_list = Manifest< Schema2::ManifestList >( dest_->copy().get(), manifest_list );
    std::atomic< size_t > thread_count       = 0;

    if ( manifest_list != dest_manifest_list ) {
      if ( manifest_list.manifests.empty() ) {
        spdlog::error( "OCI::Copy::execute Schema2::ManifestList is empty" );
      } else {
        for ( auto const &manifest : manifest_list.manifests ) {
          thread_count++;

          stm_->execute( [ &thread_count, manifest, manifest_list, this ]() -> void {
            spdlog::trace( "OCI::Copy::execute Schema2::ManifestList functor entry '{}'", manifest_list.name );

            gobha::DelayedCall     dec_count( [ &thread_count, manifest_list ]() {
              spdlog::trace( "OCI::Copy::execute '{}' finished decrementing count", manifest_list.name );
              --thread_count;
            } );
            Schema2::ImageManifest im_request;

            im_request.name            = manifest_list.name;
            im_request.requestedTarget = manifest_list.requestedTarget;
            im_request.requestedDigest = manifest.digest;

            auto image_manifest = Manifest< Schema2::ImageManifest >( src_->copy().get(), im_request );

            spdlog::trace( "OCI::Copy::execute collected Schema2::ImageManifest forward execution" );
            execute( image_manifest, manifest.digest );
          } );
        }

        spdlog::trace( "OCI::Copy::execute Schema2::ManifestList all Schema2::ImageManifests are queued" );

        while ( thread_count != 0 ) {
          using namespace std::chrono_literals;
          std::this_thread::sleep_for( 250ms );
        }

        dest_->copy()->putManifest( manifest_list, manifest_list.requestedTarget );

        spdlog::info( "OCI::Copy::execute completed ManifestList for '{}'", manifest_list.name );
      }
    }
  } catch ( std::runtime_error const &e ) {
    spdlog::error( e.what() );
  }
} // OCI::Copy Schema2::ManifestList

auto OCI::Copy::execute( Schema2::ImageManifest const &image_manifest, std::string const &target ) -> bool {
  auto src  = src_->copy();
  auto dest = dest_->copy();

  try { // FIXME: BLOCK TO BIG?
    auto dest_image_manifest = Manifest< Schema2::ImageManifest >( dest.get(), image_manifest );

    std::mutex layers_mutex;

    if ( image_manifest != dest_image_manifest ) {
      auto                layers       = image_manifest.layers;
      std::atomic< bool > layers_empty = layers.empty();
      auto                layer_itr    = layers.end();

      while ( not layers_empty ) {
        {
          spdlog::trace( "OCI::Copy::execute finding a dependant layer that is not in progress" );
          std::lock_guard< std::mutex > lg( layers_mutex );
          layer_itr = std::find_if(
              layers.begin(), layers.end(), [ &dest, image_manifest, target, this ]( const auto layer ) -> bool {
                std::lock_guard< std::mutex > lg( wd_mutex_ );
                auto wd_itr = std::find( working_digests_->begin(), working_digests_->end(), layer.digest );

                if ( wd_itr == working_digests_->end() ) {
                  if ( not dest->hasBlob( image_manifest, target, layer.digest ) ) {
                    spdlog::trace( "OCI::Copy::execute preparing to download {}", layer.digest );
                    working_digests_->push_back( layer.digest );
                  }

                  return true;
                }

                return false;
              } );
        }

        if ( layer_itr == layers.end() ) {
          // This is just a "busy" loop, with a pause, waiting for blobs to be ready before writing the manifests
          //   is there a way to "toss" this into a wait thread, so we can free this one for another operation
          //   considering the magnitude of the number of possible Projects -> tags -> blobs how do we avoid
          //   thread bombing the running host
          spdlog::trace( "OCI::Copy::execute All images being downloaded by other threads sleeping" );
          using namespace std::chrono_literals;

          std::this_thread::yield();
          std::unique_lock< std::mutex > ul( wd_mutex_ );
          finish_download_.wait_for( ul, 30s );
        } else if ( dest->hasBlob( image_manifest, target, layer_itr->digest ) ) {
          spdlog::debug( "OCI::Copy::execute Already have the layer move to next {}:{}", target, layer_itr->digest );

          std::lock_guard< std::mutex > lg( layers_mutex );
          layers.erase( layer_itr );
        } else {
          spdlog::debug( "OCI::Copy::execute layer doesn't exist, retrieving {}:{}", target, layer_itr->digest );
          auto digest     = layer_itr->digest;
          auto layer_size = layer_itr->size;

          stm_->execute( [ image_manifest, target, digest, layer_size, this ]() {
            gobha::DelayedCall finalize_thread( [ digest, this ]() {
              {
                spdlog::trace(
                    "OCI::Copy::execute (gobha::DelayedCall) Clearing {} from working digests and local layers",
                    digest );
                std::lock_guard< std::mutex > lg( wd_mutex_ );
                auto wd_itr = std::find( working_digests_->begin(), working_digests_->end(), digest );
                if ( wd_itr != working_digests_->end() ) {
                  working_digests_->erase( wd_itr );
                  finish_download_.notify_all();
                }
              }
            } );

            const auto digest_trunc = 10;
            auto       sync_bar_ref = progress_bars_->push_back(
                getIndicator( layer_size, digest.substr( digest.size() - digest_trunc ), indicators::Color::yellow ) );
            auto     dest      = dest_->copy();
            auto     src       = src_->copy();
            uint64_t data_sent = 0;

            std::function< bool( const char *, uint64_t ) > call_back =
                [ &image_manifest, &target, &dest, &digest, &layer_size, &data_sent,
                  &sync_bar_ref ]( const char *data, uint64_t data_length ) -> bool {
              if ( dest->putBlob( image_manifest, target, digest, layer_size, data, data_length ) ) {
                data_sent += data_length;

                sync_bar_ref.get().set_progress( data_sent );
                sync_bar_ref.get().set_option( indicators::option::PostfixText{ std::to_string( data_sent ) + "/" +
                                                                                std::to_string( layer_size ) } );

                return true;
              }

              spdlog::error( "OCI::Copy::execute No data recieved for layer '{}:{}' with {} bytes remaining of {}",
                             target, digest, data_sent, layer_size - data_sent, layer_size );

              return false;
            };

            spdlog::trace( "OCI::Copy::execute ready to start copy of layer '{}'", digest );

            try {
              src->fetchBlob( image_manifest.name, digest, call_back );
            } catch ( std::runtime_error const &e ) {
              spdlog::error( "Runtime Error: {}", e.what() );
            } catch ( std::exception const &e ) {
              spdlog::error( "Unspecified exception: {}", e.what() );
            }
          } );
        }

        std::lock_guard< std::mutex > lg( layers_mutex );
        layers_empty = layers.empty();
      }

      if ( not dest->hasBlob( image_manifest, target, image_manifest.config.digest ) ) {
        spdlog::debug( "OCI::Copy::execute Getting Config Blob: {}:{}", target, image_manifest.config.digest );

        uint64_t                                        data_sent = 0;
        std::function< bool( const char *, uint64_t ) > call_back =
            [ &image_manifest, &target, &data_sent, &dest ]( const char *data, uint64_t data_length ) -> bool {
          data_sent += data_length;

          return dest->putBlob( image_manifest, target, image_manifest.config.digest, image_manifest.config.size, data,
                                data_length );
        };

        src->fetchBlob( image_manifest.name, image_manifest.config.digest, call_back );
      }
    }

    return dest->putManifest( image_manifest, target );
  } catch ( std::runtime_error const &e ) {
    spdlog::error( e.what() );
  }

  return false;
} // OCI::Copy Schema2::ImageManifest

auto OCI::getIndicator( size_t max_progress, std::string const &prefix, indicators::Color color )
    -> indicators::ProgressBar {
  // clang-format off
  return indicators::ProgressBar{
      indicators::option::BarWidth{80}, // NOLINT
      indicators::option::Start{"["},
      indicators::option::Fill{"■"},
      indicators::option::Lead{"■"},
      indicators::option::Remainder{" "},
      indicators::option::End{" ]"},
      indicators::option::MaxProgress{ max_progress },
      indicators::option::ForegroundColor{ color },
      indicators::option::PrefixText{ prefix },
      indicators::option::PostfixText{ "0/" + std::to_string( max_progress ) }
  };
  // clang-format on
}
