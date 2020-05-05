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
      std::uintmax_t  size = 0;
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

    std::uint16_t           schemaVersion = 0;
    std::string             mediaType = "application/vnd.docker.distribution.manifest.list.v2+json";
    std::vector< Manifest > manifests;
    std::string             name; // <namespace>/<repo>
    std::string             originDomain; // local extension
    std::string             requestedTarget; // local extension
  };

  struct ImageManifest { // Accept: application/vnd.docker.distribution.manifest.v2+json
    struct Layer {
      std::string                 mediaType;
      std::uintmax_t              size = 0;
      std::string                 digest;
      std::vector< std::string >  urls;
    };

    std::string     name; // <namespace>/<repo>
    std::string     originDomain;    // local extension
    std::string     requestedTarget; // local extension 'Package tag/digest'
    std::string     requestedDigest; // local extension 'Digest for this Manifest'
    std::uint16_t   schemaVersion = 0;
    std::string     mediaType = "application/vnd.docker.distribution.manifest.v2+json";
    struct {
      std::string     mediaType;
      std::uintmax_t  size = 0;
      std::string     digest;
    } config;
    std::vector< Layer > layers;
  };

  auto operator==( ImageManifest const& im1, ImageManifest const& im2 ) -> bool;
  auto operator!=( ImageManifest const& im1, ImageManifest const& im2 ) -> bool;

  auto operator==( ImageManifest::Layer const& iml1, ImageManifest::Layer const& iml2 ) -> bool;
  auto operator!=( ImageManifest::Layer const& iml1, ImageManifest::Layer const& iml2 ) -> bool;

  auto operator==( ManifestList const& ml1, ManifestList const& ml2 ) -> bool;
  auto operator!=( ManifestList const& ml1, ManifestList const& ml2 ) -> bool;

  void from_json( nlohmann::json const& j, ManifestList& ml );
  void from_json( nlohmann::json const& j, ManifestList::Manifest& mlm );

  void to_json( nlohmann::json& j, ManifestList const& ml );
  void to_json( nlohmann::json& j, ManifestList::Manifest const& mlm );

  void from_json( nlohmann::json const& j, ImageManifest& im );
  void from_json( nlohmann::json const& j, ImageManifest::Layer& iml );

  void to_json( nlohmann::json& j, ImageManifest const& im );
  void to_json( nlohmann::json& j, ImageManifest::Layer const& iml );
}// namespace OCI::Schema2
