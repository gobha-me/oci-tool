#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp> // https://github.com/nlohmann/json
#include <iostream>

namespace OCI::Schema2 {
  // https://docs.docker.com/registry/spec/api/
  // Manifestv2, Schema2 implementation, https://docs.docker.com/registry/spec/manifest-v2-2/
  struct ManifestList {  // Accept: application/vnd.docker.distribution.manifest.list.v2+json
    struct Manifest {
      std::string     mediaType;
      std::uintmax_t  size;
      std::string     digest;

      struct {
        std::string                 architecture;
        std::string                 os;
        std::string                 os_version;
        std::vector< std::string >  os_features;
        std::string                 variant;
        std::vector< std::string >  features;
      } platform;
    };

    std::uint16_t           schemaVersion;
    std::string             mediaType = "application/vnd.docker.distribution.manifest.list.v2+json";
    std::vector< Manifest > manifests;
    std::string             name; // <namespace>/<repo>
  };

  struct ImageManifest { // Accept: application/vnd.docker.distribution.manifest.v2+json
    struct Layer {
      std::string                 mediaType;
      std::uintmax_t              size;
      std::string                 digest;
      std::vector< std::string >  urls;
    };

    std::string     name; // <namespace>/<repo>
    std::uint16_t   schemaVersion;
    std::string     mediaType = "application/vnd.docker.distribution.manifest.v2+json";
    struct {
      std::string     mediaType;
      std::uintmax_t  size;
      std::string     digest;
    } config;
    std::vector< Layer > layers;
  };

  void from_json( const nlohmann::json& j, ManifestList& ml );
  void from_json( const nlohmann::json& j, ManifestList::Manifest& mlm );

  void to_json( nlohmann::json& j, const ManifestList& ml );
  void to_json( nlohmann::json& j, const ManifestList::Manifest& mlm );

  void from_json( const nlohmann::json& j, ImageManifest& im );
  void from_json( const nlohmann::json& j, ImageManifest::Layer& iml );

  void to_json( nlohmann::json& j, const ImageManifest& im );
  void to_json( nlohmann::json& j, const ImageManifest::Layer& iml );
}// namespace OCI::Schema2

// IMPLEMENTATION
// OCI::Schema2
void OCI::Schema2::from_json( const nlohmann::json& j, ManifestList& ml ) {
  j.at( "schemaVersion" ).get_to( ml.schemaVersion );

  if ( j.find( "manifests" ) != j.end() )
    ml.manifests = j.at( "manifests" ).get< std::vector< ManifestList::Manifest > >();
}

void OCI::Schema2::from_json( const nlohmann::json& j, ManifestList::Manifest& mlm ) {
  j.at( "mediaType" ).get_to( mlm.mediaType );
  j.at( "size" ).get_to( mlm.size );
  j.at( "digest" ).get_to( mlm.digest );
  j.at( "platform" ).at( "architecture" ).get_to( mlm.platform.architecture );
  j.at( "platform" ).at( "os" ).get_to( mlm.platform.os );

  if ( j.at( "platform" ).find( "os.version" ) != j.at( "platform" ).end() )
    j.at( "platform" ).at( "os.version" ).get_to( mlm.platform.os_version );

  if ( j.at( "platform" ).find( "os.features" ) != j.at( "platform" ).end() )
    j.at( "platform" ).at( "os.features" ).get_to( mlm.platform.os_features );

  if ( j.at( "platform" ).find( "variant" ) != j.at( "platform" ).end() )
    j.at( "platform" ).at( "variant" ).get_to( mlm.platform.variant );

  if ( j.at( "platform" ).find( "features" ) != j.at( "platform" ).end() )
    j.at( "platform" ).at( "features" ).get_to( mlm.platform.features );
}

void OCI::Schema2::to_json( nlohmann::json& j, const ManifestList& ml ) {
  (void)j;
  (void)ml;

  std::cout << "Construct Json from OCI::Schema2::ManifestList" << std::endl;
}

void OCI::Schema2::to_json( nlohmann::json& j, const ManifestList::Manifest& mlm ) {
  (void)j;
  (void)mlm;

  std::cout << "Construct Json from OCI::Schema2::ManifestList::Manifest" << std::endl;
}

void OCI::Schema2::from_json( const nlohmann::json& j, ImageManifest& im ) {
  j.at( "schemaVersion" ).get_to( im.schemaVersion );

  if ( j.find( "mediaType" ) != j.end() )
    j.at( "mediaType" ).get_to( im.mediaType );

  if ( j.find( "config" ) != j.end() ) {
    j.at( "config" ).at( "mediaType" ).get_to( im.config.mediaType );
    j.at( "config" ).at( "size" ).get_to( im.config.size );
    j.at( "config" ).at( "digest" ).get_to( im.config.digest );
  }

  if ( j.find( "layers" ) != j.end() ) 
    im.layers = j.at( "layers" ).get< std::vector< ImageManifest::Layer > >();
}

void OCI::Schema2::from_json( const nlohmann::json& j, ImageManifest::Layer& iml ) {
  j.at( "mediaType" ).get_to( iml.mediaType );
  j.at( "size" ).get_to( iml.size );
  j.at( "digest" ).get_to( iml.digest );

  if ( j.find( "urls" ) != j.end() )
    j.at( "urls" ).get_to( iml.digest );
}

void OCI::Schema2::to_json( nlohmann::json& j, const ImageManifest& im ) {
  (void)j;
  (void)im;
}

void OCI::Schema2::to_json( nlohmann::json& j, const  ImageManifest::Layer& iml ) {
  (void)j;
  (void)iml;
}
