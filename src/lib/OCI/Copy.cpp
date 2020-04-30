#include <OCI/Copy.hpp>

void OCI::Copy( const std::string& rsrc, const std::string& target, OCI::Base::Client* src, OCI::Base::Client* dest ) {
  auto manifest_list = Manifest< Schema2::ManifestList >( src, rsrc, target );

  if ( manifest_list.schemaVersion == 1 ) { // Fall back to Schema1
    auto image_manifest = Manifest< Schema1::ImageManifest >( src, rsrc, target );

    Copy( image_manifest, target, src, dest );
  } else {
    Copy( manifest_list, target, src, dest );
  }
}

void OCI::Copy( const Schema1::ImageManifest& image_manifest, const std::string& target, OCI::Base::Client* src, OCI::Base::Client* dest ) {
  (void)src;
  (void)target;

  for ( auto const& layer: image_manifest.fsLayers ) {
    if ( layer.first == "blobSum" ) {
      if ( not dest->hasBlob( image_manifest, layer.second ) ) {
        std::cout << "Destintaion doesn't have layer" << std::endl;
      }
    }
  }

  std::cout << "Test is successful and Post Schema1::ImageManifest to OCI::Base::Client::putManifest" << std::endl;
}

void OCI::Copy( Schema2::ManifestList& manifest_list, std::string const& target, OCI::Base::Client* src, OCI::Base::Client* dest ) {
  bool success = true;

  for ( auto& manifest: manifest_list.manifests ) {
    auto image_manifest = Manifest< Schema2::ImageManifest >( src, manifest_list.name, manifest.digest );

    success = success and Copy( image_manifest, manifest.digest, src, dest );
  }

  if ( success ) {
    std::cout << "All ImageManifests for ManifestList: " << target << " have been put to target Client." << std::endl;
    dest->putManifest( manifest_list, target );
  }
}

auto OCI::Copy( Schema2::ImageManifest const& image_manifest, std::string& target, OCI::Base::Client* src, OCI::Base::Client* dest ) -> bool {
  bool retVal              = true;
  auto dest_image_manifest = Manifest< Schema2::ImageManifest >( dest, image_manifest.name, target );

  for ( auto const& layer: image_manifest.layers ) {
    if ( src->hasBlob( image_manifest, target, layer.digest ) and not dest->hasBlob( image_manifest, target, layer.digest ) ) {
      uint64_t data_sent = 0;
      std::function< bool( const char *, uint64_t ) > call_back = [&]( const char *data, uint64_t data_length ) -> bool {
        data_sent += data_length;

        return dest->putBlob( image_manifest, target, layer.digest, layer.size, data, data_length );
      };

      retVal = retVal and src->fetchBlob( image_manifest.name, layer.digest, call_back );
    }
  }

  if ( retVal ) {
    uint64_t data_sent = 0;
    std::function< bool( const char *, uint64_t ) > call_back = [&]( const char *data, uint64_t data_length ) -> bool {
      data_sent += data_length;

      return dest->putBlob( image_manifest, target, image_manifest.config.digest, image_manifest.config.size, data, data_length );
    };

    retVal = retVal and src->fetchBlob( image_manifest.name, image_manifest.config.digest, call_back );

    if ( retVal ) {
      std::cout << "All images for ImageManifest: " << target << " have been put to target Client." << std::endl;
      retVal = dest->putManifest( image_manifest, target ); // putManifest defines this
    }
  }

  return retVal;
}
