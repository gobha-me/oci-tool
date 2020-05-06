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
    Copy( manifest_list, src, dest );
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
  bool changed = false;

  Schema2::ImageManifest im_request;
  im_request.name            = manifest_list.name;
  im_request.requestedTarget = manifest_list.requestedTarget;

  for ( auto& manifest: manifest_list.manifests ) {
    im_request.requestedDigest = manifest.digest;

    auto image_manifest = Manifest< Schema2::ImageManifest >( src, im_request );

    if ( Copy( image_manifest, manifest.digest, src, dest ) ) {
      changed = true;
    }
  }

  // if not changed, is it just missing the ManifestList, need to validate

  if ( changed ) {
    std::cout << "ImageManifests updated for ManifestList: " << manifest_list.name << ":" << manifest_list.requestedTarget << " have been put to target Client." << std::endl;
    dest->putManifest( manifest_list, manifest_list.requestedTarget );
  }
} // OCI::Copy Schema2::ManifestList

auto OCI::Copy( Schema2::ImageManifest const& image_manifest, std::string& target, OCI::Base::Client* src, OCI::Base::Client* dest ) -> bool {
  bool retVal  = true;
  bool newBlob = false;

  auto dest_image_manifest = Manifest< Schema2::ImageManifest >( dest, image_manifest );
  std::vector< std::future< bool > > processes;

  processes.reserve( image_manifest.layers.size() );

  nlohmann::json j = dest_image_manifest;

  if ( image_manifest.layers == dest_image_manifest.layers ) {
    //std::cout << "ImageManifests are equal" << std::endl;
    retVal = false; // no change
  } else {
    for ( auto const& layer: image_manifest.layers ) {
      if ( not dest->hasBlob( image_manifest, target, layer.digest ) ) {
        newBlob = true;

        processes.push_back( std::async( std::launch::async, [&]() -> bool {
          bool lretVal     = true;
          auto source      = src->copy();
          auto destination = dest->copy();

          uint64_t data_sent = 0;
          std::function< bool( const char *, uint64_t ) > call_back = [&]( const char *data, uint64_t data_length ) -> bool {
            data_sent += data_length;

            return destination->putBlob( image_manifest, target, layer.digest, layer.size, data, data_length );
          };

          lretVal = source->fetchBlob( image_manifest.name, layer.digest, call_back );

          return lretVal;
        } ) );
      }
    }
  }

  if ( not dest->hasBlob( image_manifest, target, image_manifest.config.digest ) ) {
    std::cout << "Getting Config Blob: " << image_manifest.config.digest << std::endl;
    newBlob = true;

    uint64_t data_sent = 0;
    std::function< bool( const char *, uint64_t ) > call_back = [&]( const char *data, uint64_t data_length ) -> bool {
      data_sent += data_length;

      return dest->putBlob( image_manifest, target, image_manifest.config.digest, image_manifest.config.size, data, data_length );
    };

    retVal = retVal and src->fetchBlob( image_manifest.name, image_manifest.config.digest, call_back );
  }


  for ( auto& process : processes ) {
    process.wait();
    retVal = retVal and process.get();
  }

  retVal = retVal and newBlob;

  if ( retVal and dest->putManifest( image_manifest, target ) ) {
    std::cout << "Set of images in ImageManifest:" << image_manifest.name << ":" << image_manifest.requestedTarget  << " have been put to target Client." << std::endl;
  } else {
    retVal = false;
  }

  return retVal;
} // OCI::Copy Schema2::ImageManifest
