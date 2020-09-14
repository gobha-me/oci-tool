#include <OCI/Copy.hpp>
#include <future>
#include <spdlog/spdlog.h>
#include <vector>

void OCI::Copy( std::string const &rsrc, std::string const &target, OCI::Base::Client *src, OCI::Base::Client *dest,
                ProgressBars &progress_bars ) {
  Schema2::ManifestList ml_request;

  ml_request.name            = rsrc;
  ml_request.requestedTarget = target;

  auto manifest_list = Manifest< Schema2::ManifestList >( src, ml_request );

  switch ( manifest_list.schemaVersion ) {
  case 1: // Fall back to Schema1
  {
    Schema1::ImageManifest im_request;

    im_request.name            = rsrc;
    im_request.requestedTarget = target;

    auto image_manifest = Manifest< Schema1::ImageManifest >( src, im_request );

    if ( not image_manifest.fsLayers.empty() ) {
      spdlog::info( "OCI::Copy Start Schema1 ImageManifest {}:{}", rsrc, target );
      Copy( image_manifest, src, dest, progress_bars );
      spdlog::info( "OCI::Copy Finish Schema1 ImageManifest {}:{}", rsrc, target );
    }
  }

  break;
  case 2:
    if ( not manifest_list.manifests.empty() ) {
      spdlog::info( "OCI::Copy Start Schema2 ManifestList {}:{}", rsrc, target );
      Copy( manifest_list, src, dest, progress_bars );
      spdlog::info( "OCI::Copy Finish Schema2 ManifestList {}:{}", rsrc, target );
    }

    break;
  default:
    spdlog::error( "Unknown schemaVersion {}", manifest_list.schemaVersion );
  }
}

void OCI::Copy( const Schema1::ImageManifest &image_manifest, OCI::Base::Client *src, OCI::Base::Client *dest,
                ProgressBars &progress_bars ) {
  (void)src;
  (void)progress_bars;

  for ( auto const &layer : image_manifest.fsLayers ) {
    if ( layer.first == "blobSum" ) {
      if ( not dest->hasBlob( image_manifest, layer.second ) ) {
        spdlog::info( "Destintaion doesn't have layer" );
      }
    }
  }

  spdlog::warn( "Test is successful and Post Schema1::ImageManifest to OCI::Base::Client::putManifest" );
}

void OCI::Copy( const Schema1::SignedImageManifest &image_manifest, OCI::Base::Client *src, OCI::Base::Client *dest,
                ProgressBars &progress_bars ) {
  (void)src;
  (void)progress_bars;

  for ( auto const &layer : image_manifest.fsLayers ) {
    if ( layer.first == "blobSum" ) {
      if ( not dest->hasBlob( image_manifest, layer.second ) ) {
        spdlog::info( "Destintaion doesn't have layer" );
      }
    }
  }

  spdlog::warn( "Test is successful and Post Schema1::ImageManifest to OCI::Base::Client::putManifest" );
}

void OCI::Copy( Schema2::ManifestList &manifest_list, OCI::Base::Client *src, OCI::Base::Client *dest,
                ProgressBars &progress_bars ) {
  auto dest_manifest_list = Manifest< Schema2::ManifestList >( dest, manifest_list );

  if ( manifest_list != dest_manifest_list ) {
    std::vector< std::thread > processes;

    processes.reserve( manifest_list.manifests.size() );

    for ( auto &manifest : manifest_list.manifests ) {
      processes.emplace_back( [ & ]() -> void {
        Schema2::ImageManifest im_request;

        auto local_manifest_list   = manifest_list;
        im_request.name            = local_manifest_list.name;
        im_request.requestedTarget = local_manifest_list.requestedTarget;
        im_request.requestedDigest = manifest.digest;

        auto source         = src->copy();
        auto destination    = dest->copy();
        auto image_manifest = Manifest< Schema2::ImageManifest >( source.get(), im_request );

        Copy( image_manifest, manifest.digest, source.get(), destination.get(), progress_bars );
      } );
    }

    for ( auto &process : processes ) {
      process.join();
    }

    dest->putManifest( manifest_list, manifest_list.requestedTarget );
  }
} // OCI::Copy Schema2::ManifestList

std::mutex WD_MUTEX;
auto OCI::Copy( Schema2::ImageManifest const &image_manifest, std::string &target, OCI::Base::Client *src,
                OCI::Base::Client *dest, ProgressBars &progress_bars ) -> bool {
  auto dest_image_manifest = Manifest< Schema2::ImageManifest >( dest, image_manifest );
  static std::vector< std::string > working_digests;
  auto layers = image_manifest.layers;

  if ( image_manifest != dest_image_manifest ) {
    while ( not layers.empty() ) {
      // Find image not being operated on by another thread
      auto layer_itr = std::find_if( layers.begin(), layers.begin(), [&]( auto layer ) -> bool {
          std::lock_guard< std::mutex > lg( WD_MUTEX );
          auto wd_itr = std::find( working_digests.begin(), working_digests.end(), layer.digest );

          if ( wd_itr == working_digests.end() ) {
            working_digests.emplace_back( layer.digest );

            return true;
          }

          return false;
        } );

      if ( layer_itr == layers.end() ) {
        // All images being downloaded by other threads
        using namespace std::chrono_literals;
        std::this_thread::sleep_for( 50ms );
      } else if ( dest->hasBlob( image_manifest, target, layer_itr->digest ) ) {
        // All ready have the layer move to next
        {
          std::lock_guard< std::mutex > lg( WD_MUTEX );
          auto wd_itr = std::find( working_digests.begin(), working_digests.end(), layer_itr->digest );
          if ( wd_itr != working_digests.end() ) {
            working_digests.erase( wd_itr );
          }
        }

        layers.erase( layer_itr );
      } else {
        // clang-format off
        indicators::ProgressBar sync_bar{
            indicators::option::BarWidth{60}, // NOLINT
            indicators::option::Start{"["},
            indicators::option::Fill{"■"},
            indicators::option::Lead{"■"},
            indicators::option::Remainder{" "},
            indicators::option::End{" ]"},
            indicators::option::MaxProgress{ layer_itr->size },
            indicators::option::ForegroundColor{indicators::Color::yellow},
            indicators::option::PrefixText{ layer_itr->digest.substr( layer_itr->digest.size() - 10 ) }, // NOLINT(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
            indicators::option::PostfixText{ "0/" + std::to_string( layer_itr->size ) }
          };
        // clang-format on
        auto sync_bar_ref = progress_bars.push_back( sync_bar );

        uint64_t                                        data_sent = 0;
        std::function< bool( const char *, uint64_t ) > call_back = [ & ]( const char *data,
                                                                           uint64_t    data_length ) -> bool {
          data_sent += data_length;

          if ( dest->putBlob( image_manifest, target, layer_itr->digest, layer_itr->size, data, data_length ) ) {
            sync_bar_ref.get().set_progress( data_sent );
            sync_bar_ref.get().set_option(
                indicators::option::PostfixText{ std::to_string( data_sent ) + "/" + std::to_string( layer_itr->size ) } );

            return true;
          }

          return false;
        };

        src->fetchBlob( image_manifest.name, layer_itr->digest, call_back );

        {
          std::lock_guard< std::mutex > lg( WD_MUTEX );
          auto wd_itr = std::find( working_digests.begin(), working_digests.end(), layer_itr->digest );
          if ( wd_itr != working_digests.end() ) {
            working_digests.erase( wd_itr );
          }
        }

        layers.erase( layer_itr );
      }
    }

    while ( true ) {
      {
        std::lock_guard< std::mutex > lg( WD_MUTEX );
        auto wd_itr = std::find( working_digests.begin(), working_digests.end(), image_manifest.config.digest );

        if ( wd_itr == working_digests.end() ) {
          working_digests.emplace_back( image_manifest.config.digest );

          break;
        }
      }

      using namespace std::chrono_literals;
      std::this_thread::sleep_for( 50ms );
    }

    if ( not dest->hasBlob( image_manifest, target, image_manifest.config.digest ) ) {
      spdlog::info( "Getting Config Blob: {}", image_manifest.config.digest );

      uint64_t                                        data_sent = 0;
      std::function< bool( const char *, uint64_t ) > call_back = [ & ]( const char *data,
                                                                         uint64_t    data_length ) -> bool {
        data_sent += data_length;

        return dest->putBlob( image_manifest, target, image_manifest.config.digest, image_manifest.config.size, data,
                              data_length );
      };

      src->fetchBlob( image_manifest.name, image_manifest.config.digest, call_back );
    }

    std::lock_guard< std::mutex > lg( WD_MUTEX );
    auto wd_itr = std::find( working_digests.begin(), working_digests.end(), image_manifest.config.digest );
    if ( wd_itr != working_digests.end() ) {
      working_digests.erase( wd_itr );
    }

  }

  return dest->putManifest( image_manifest, target );
} // OCI::Copy Schema2::ImageManifest
