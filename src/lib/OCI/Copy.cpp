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

void OCI::Copy( const Schema2::ManifestList& manifest_list, const std::string& target, OCI::Base::Client* src, OCI::Base::Client* dest ) {
  (void)target;
  for ( auto const& manifest: manifest_list.manifests ) {
    auto image_manifest = Manifest< Schema2::ImageManifest >( src, manifest_list.name, manifest.digest );

    //std::cout << image_manifest.name << " " << manifest.digest << std::endl;
    Copy( image_manifest, manifest.digest, src, dest );
  }
  
  // When the above is finished post ManifestList <target> to OCI::Base::Client*

  std::cout << "Test is successful and Post Schema2::ManifestList to OCI::Base::Client::putManifest" << std::endl;
}

void OCI::Copy( const Schema2::ImageManifest& image_manifest, const std::string& target, OCI::Base::Client* src, OCI::Base::Client* dest ) {
  auto dest_image_manifest = Manifest< Schema2::ImageManifest >( dest, image_manifest.name, target );

  if ( image_manifest != dest_image_manifest ) {
    for ( auto const& layer: image_manifest.layers ) {
      if ( not dest->hasBlob( image_manifest, layer.digest ) ) {
        std::function< void( const char *, uint64_t ) > call_back = [&]( const char *data, uint64_t data_length ) {
          dest->putBlob( image_manifest, target, layer.size, data, data_length );
        };

        src->fetchBlob( image_manifest.name, layer.digest, call_back );
      }
    }

    //if ( true ) { // FIXME: define success criteria for above, or do we just do it?
      dest->putManifest( image_manifest, image_manifest.name, target );
    //}
  }
}
