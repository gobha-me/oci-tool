#pragma once
#include <string>
#include <OCI/Base/Client.hpp>
#include <OCI/Schema1.hpp>
#include <OCI/Schema2.hpp>
#include <OCI/Manifest.hpp>

namespace OCI {
  void Copy( const std::string& rsrc, const std::string& target, OCI::Base::Client* src, OCI::Base::Client* dest );

  void Copy( Schema1::ImageManifest image_manifest, OCI::Base::Client* src, OCI::Base::Client* dest );

  void Copy( const Schema2::ManifestList& manifest_list, OCI::Base::Client* src, OCI::Base::Client* dest );

  void Copy( Schema2::ImageManifest image_manifest, OCI::Base::Client* src, OCI::Base::Client* dest );

  void Sync( const std::string& rsrc, OCI::Base::Client* src, OCI::Base::Client* dest );

  void Sync( const std::string& rsrc, std::vector< std::string > tags, OCI::Base::Client* src, OCI::Base::Client* dest );
} // namespace OCI


// Implementatiion
void OCI::Copy( const std::string& rsrc, const std::string& target, OCI::Base::Client* src, OCI::Base::Client* dest ) {
  Schema2::ManifestList manifest_list = Manifest< Schema2::ManifestList >( src, rsrc, target );

  if ( manifest_list.schemaVersion == 1 ) { // Fall back to Schema1
    Schema1::ImageManifest image_manifest = Manifest< Schema1::ImageManifest >( src, rsrc, target );

    Copy( image_manifest, src, dest );
  } else {
    Copy( manifest_list, src, dest );
  }
}

void OCI::Copy( Schema1::ImageManifest image_manifest, OCI::Base::Client* src, OCI::Base::Client* dest ) {
  (void)src;

  for ( auto layer: image_manifest.fsLayers ) {
    if ( layer.first == "blobSum" ) {
      if ( not dest->hasBlob( layer.second ) ) {
      }
    }
  }
}

void OCI::Copy( const Schema2::ManifestList& manifest_list, OCI::Base::Client* src, OCI::Base::Client* dest ) {
  for ( auto manifest_: manifest_list.manifests ) {
    Schema2::ImageManifest image_manifest = Manifest< Schema2::ImageManifest >( src, manifest_list.name, manifest_.digest );

    Copy( image_manifest, src, dest );
  }
  
  // When the above is finished post ManifestList to OCI::Base::Client*
}

void OCI::Copy( Schema2::ImageManifest image_manifest, OCI::Base::Client* src, OCI::Base::Client* dest ) {
  (void)src;

  std::cout << image_manifest.name << std::endl;
  for ( auto layer: image_manifest.layers ) {
    std::cout << layer.digest << std::endl;
    if ( not dest->hasBlob( layer.digest ) ) {
      // if not copy blob from src to dest
    }
  }

  // When the above is finished post ImageManifest to OCI::Base::Client*
}

void OCI::Sync( const std::string& rsrc, OCI::Base::Client* src, OCI::Base::Client* dest ) {
  auto tagList = src->tagList( rsrc );

  for ( auto tag: tagList.tags )
    Copy( tagList.name, tag, src, dest );
}

void OCI::Sync( const std::string& rsrc, std::vector< std::string > tags, OCI::Base::Client* src, OCI::Base::Client* dest ) {
  for ( auto tag: tags )
    Copy( rsrc, tag, src, dest );
}
