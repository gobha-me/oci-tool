#include <OCI/Copy.hpp>
#include <future>
#include <vector>

void OCI::Copy( std::string const& rsrc, std::string const& target, OCI::Base::Client* src, OCI::Base::Client* dest ) {
  Schema2::ManifestList ml_request;

  ml_request.name            = rsrc;
  ml_request.requestedTarget = target;

  auto manifest_list = Manifest< Schema2::ManifestList >( src, ml_request );

  if ( manifest_list.schemaVersion == 1 ) { // Fall back to Schema1
    Schema1::ImageManifest im_request;

    im_request.name            = rsrc;
    im_request.requestedTarget = target;

    auto image_manifest = Manifest< Schema1::ImageManifest >( src, im_request );

    Copy( image_manifest, src, dest );
  } else {
    std::cout << "OCI::Copy Start  " << rsrc << ":" << target << "\n";
    Copy( manifest_list, src, dest );
    std::cout << "OCI::Copy Finish " << rsrc << ":" << target << "\n";
  }
}

void OCI::Copy( const Schema1::ImageManifest& image_manifest, OCI::Base::Client* src, OCI::Base::Client* dest ) {
  (void)src;

  for ( auto const& layer: image_manifest.fsLayers ) {
    if ( layer.first == "blobSum" ) {
      if ( not dest->hasBlob( image_manifest, layer.second ) ) {
        std::cout << "Destintaion doesn't have layer" << std::endl;
      }
    }
  }

  std::cout << "Test is successful and Post Schema1::ImageManifest to OCI::Base::Client::putManifest" << std::endl;
}

void OCI::Copy( Schema2::ManifestList& manifest_list, OCI::Base::Client* src, OCI::Base::Client* dest ) {
  auto dest_manifest_list    = Manifest< Schema2::ManifestList >( dest, manifest_list );

  if ( manifest_list != dest_manifest_list ) {
    std::vector< std::thread > processes;

    processes.reserve( manifest_list.manifests.size() );

    for ( auto& manifest: manifest_list.manifests ) {
      processes.emplace_back( [&]() -> void {
        Schema2::ImageManifest im_request;

        auto local_manifest_list   = manifest_list;
        im_request.name            = local_manifest_list.name;
        im_request.requestedTarget = local_manifest_list.requestedTarget;
        im_request.requestedDigest = manifest.digest;

        auto source         = src->copy();
        auto destination    = dest->copy();
        auto image_manifest = Manifest< Schema2::ImageManifest >( source.get(), im_request );

        Copy( image_manifest, manifest.digest, source.get(), destination.get() );
      } );
    }

    for ( auto& process : processes ) {
      process.join();
    }

    dest->putManifest( manifest_list, manifest_list.requestedTarget );
  }
} // OCI::Copy Schema2::ManifestList

auto OCI::Copy( Schema2::ImageManifest const& image_manifest, std::string& target, OCI::Base::Client* src, OCI::Base::Client* dest ) -> bool {
  auto dest_image_manifest = Manifest< Schema2::ImageManifest >( dest, image_manifest );

  if ( image_manifest != dest_image_manifest ) {
    for ( auto const& layer: image_manifest.layers ) {
      if ( not dest->hasBlob( image_manifest, target, layer.digest ) ) {
        uint64_t data_sent = 0;
        std::function< bool( const char *, uint64_t ) > call_back = [&]( const char *data, uint64_t data_length ) -> bool {
          data_sent += data_length;

          return dest->putBlob( image_manifest, target, layer.digest, layer.size, data, data_length );
        };

        src->fetchBlob( image_manifest.name, layer.digest, call_back );
      }
    }

    if ( not dest->hasBlob( image_manifest, target, image_manifest.config.digest ) ) {
      std::cout << "Getting Config Blob: " << image_manifest.config.digest << "\n";

      uint64_t data_sent = 0;
      std::function< bool( const char *, uint64_t ) > call_back = [&]( const char *data, uint64_t data_length ) -> bool {
        data_sent += data_length;

        return dest->putBlob( image_manifest, target, image_manifest.config.digest, image_manifest.config.size, data, data_length );
      };

      src->fetchBlob( image_manifest.name, image_manifest.config.digest, call_back );
    }
  }

  return dest->putManifest( image_manifest, target );
} // OCI::Copy Schema2::ImageManifest
