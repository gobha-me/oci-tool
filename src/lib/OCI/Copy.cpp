#include <OCI/Copy.hpp>

void OCI::Copy( const std::string& rsrc, const std::string& target, OCI::Base::Client* src, OCI::Base::Client* dest ) {
  auto manifest_list = Manifest< Schema2::ManifestList >( src, rsrc, target );

  if ( manifest_list.schemaVersion == 1 ) { // Fall back to Schema1
    auto image_manifest = Manifest< Schema1::ImageManifest >( src, rsrc, target );

    Copy( image_manifest, src, dest );
  } else {
    Copy( manifest_list, src, dest );
  }
}

void OCI::Copy( const Schema1::ImageManifest& image_manifest, OCI::Base::Client* src, OCI::Base::Client* dest ) {
  (void)src;

  for ( auto const& layer: image_manifest.fsLayers ) {
    if ( layer.first == "blobSum" ) {
      if ( not dest->hasBlob( image_manifest.name, layer.second ) ) {
        std::cout << "Destintaion doesn't have layer" << std::endl;
      }
    }
  }
}

void OCI::Copy( const Schema2::ManifestList& manifest_list, OCI::Base::Client* src, OCI::Base::Client* dest ) {
  for ( auto const& manifest_: manifest_list.manifests ) {
    auto image_manifest = Manifest< Schema2::ImageManifest >( src, manifest_list.name, manifest_.digest );

    std::cout << image_manifest.name << " " << manifest_.digest << std::endl;
    Copy( image_manifest, src, dest );
  }
  
  // When the above is finished post ManifestList to OCI::Base::Client*
}

void OCI::Copy( const Schema2::ImageManifest& image_manifest, OCI::Base::Client* src, OCI::Base::Client* dest ) {
  (void)src;

  for ( auto const& layer: image_manifest.layers ) {
    if ( not dest->hasBlob( image_manifest.name, layer.digest ) ) {
      std::function< void( const char *, uint64_t ) > call_back = [&](const char *data, uint64_t data_length){
        (void)data;
        (void)data_length;
        std::cout << "Destintaion doesn't have layer: " << layer.digest << std::endl;
        std::cout << "Size: " << layer.size << std::endl;
      };

      src->fetchBlob( image_manifest.name, layer.digest, call_back );
    }
  }

  // When the above is finished post ImageManifest to OCI::Base::Client*
}
