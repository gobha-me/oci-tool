#pragma once
#include <iostream>
#include <nlohmann/json.hpp> // https://github.com/nlohmann/json
#include <string>
#include <utility>
#include <vector>

namespace OCI::Schema1 { // https://docs.docker.com/registry/spec/api/
  struct ImageManifest {
    const std::string mediaType = "application/vnd.docker.distribution.manifest.v1+json"; // not part of the json object, this is part of the interface
    std::string       name;
    std::string       originDomain;
    std::string       requestedTarget;
    std::string       tag;
    std::string       architecture;
    std::string       schemaVersion;
    std::string       raw_str;
    std::vector< std::pair< std::string, std::string > > fsLayers;
    std::vector< std::pair< std::string, std::string > > history;
  };

  struct SignedImageManifest : ImageManifest {
    struct Signature { // can be implemented at some point
      struct { // http://self-issued.info/docs/draft-ietf-jose-json-web-signature.html
      } header;
      std::string signature;
      std::string protected_; // protected is a keyword :P
    };

    const std::string         mediaType = "application/vnd.docker.distribution.manifest.v1+prettyjws";
    std::vector< Signature >  signatures;
  };

  void from_json( const nlohmann::json& j, ImageManifest& im );
  void from_json( const nlohmann::json& j, SignedImageManifest& sim );
  void from_json( const nlohmann::json& j, SignedImageManifest::Signature& sims );

  void to_json( nlohmann::json& j, ImageManifest const& im );
} // namespace OCI::Schema1
