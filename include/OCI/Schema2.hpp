#pragma once
#include <iostream>
#include <nlohmann/json.hpp> // https://github.com/nlohmann/json
#include <string>
#include <vector>

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
