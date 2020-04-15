#pragma once
#include <string>
#include <OCI/Schema1.hpp>
#include <OCI/Schema2.hpp>

namespace OCI {
  template< class SRC, class DEST >
  void Copy( const std::string& rsrc, const std::string& target, SRC src, DEST dest );

  template< class SRC, class DEST >
  void Copy( Schema1::ImageManifest image_manifest, SRC src, DEST dest );

  template< class SRC, class DEST >
  void Copy( const Schema2::ManifestList& manifest_list, SRC src, DEST dest );

  template< class SRC, class DEST >
  void Copy( Schema2::ImageManifest image_manifest, SRC src, DEST dest );

  template< class SRC, class DEST >
  void Sync( const std::string& rsrc, SRC src, DEST dest );

  template< class SRC, class DEST >
  void Sync( const std::string& rsrc, std::vector< std::string > tags, SRC src, DEST dest );
} // namespace OCI


// Implementatiion
template< class SRC, class DEST >
void OCI::Copy( const std::string& rsrc, const std::string& target, SRC src, DEST dest ) {
  Schema2::ManifestList manifest_list = src.template manifest< Schema2::ManifestList >( rsrc, target );

  if ( manifest_list.schemaVersion == 1 ) { // Fall back to Schema1
    Schema1::ImageManifest image_manifest = src.template manifest< Schema1::ImageManifest >( rsrc, target );

    Copy( image_manifest, src, dest );
  } else {
    Copy( manifest_list, src, dest );
  }
}

template< class SRC, class DEST >
void OCI::Copy( Schema1::ImageManifest image_manifest, SRC src, DEST dest ) {
  (void)image_manifest;
  (void)src;
  (void)dest;
}

template< class SRC, class DEST >
void OCI::Copy( const Schema2::ManifestList& manifest_list, SRC src, DEST dest ) {
  for ( auto manifest_: manifest_list.manifests ) {
    Schema2::ImageManifest image_manifest = src.template manifest< Schema2::ImageManifest >( manifest_list.name, manifest_.digest );

    Copy( image_manifest, src, dest );
  }
  
  // When the above is finished post ManifestList to DEST
}

template< class SRC, class DEST >
void OCI::Copy( Schema2::ImageManifest image_manifest, SRC src, DEST dest ) {
  (void)image_manifest;
  (void)src;
  (void)dest;

  std::cout << image_manifest.name << std::endl;
  for ( auto layer: image_manifest.layers ) {
    std::cout << layer.digest << std::endl;
  }

  // When the above is finished post ImageManifest to DEST
}

template< class SRC, class DEST >
void OCI::Sync( const std::string& rsrc, SRC src, DEST dest ) {
  auto tagList = src.tagList( rsrc );

  for ( auto tag: tagList.tags )
    Copy( tagList.name, tag, src, dest );
}

template< class SRC, class DEST >
void OCI::Sync( const std::string& rsrc, std::vector< std::string > tags, SRC src, DEST dest ) {
  for ( auto tag: tags )
    Copy( rsrc, tag, src, dest );
}
