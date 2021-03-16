#pragma once
#include <iostream>
#include <nlohmann/json.hpp> // https://github.com/nlohmann/json
#include <string>
#include <utility>
#include <vector>

namespace OCI::Schema1 {
  // https://docs.docker.com/registry/spec/api/
  // Manifestv2, Schema1 implementation, https://docs.docker.com/registry/spec/manifest-v2-1/
  struct ImageManifest {
    const std::string mediaType =
        "application/vnd.docker.distribution.manifest.v1+json"; // not part of the json object, this is part of the
                                                                // interface
    std::string                                          name; // image repo tag
    std::string                                          tag; // image tag CODEUSED
    std::string                                          architecture; // arch for the image, info purposes only CODEUSED
    std::vector< std::pair< std::string, std::string > > fsLayers; // blobSum digest of filesystem image layers. SHA256 CODEUSED
    std::vector< std::pair< std::string, std::string > > history; // array of v1Compatibility strings
    std::int                                             schemaVersion; // image manifest schema
    std::string                                          raw_str; // raw string of entire JSON document
  };

  struct SignedImageManifest : ImageManifest {
    struct Signature { // can be implemented at some point
        struct {
          struct {
          std::string                                          crv; // Curve parameter
          std::string                                          kid; // Key ID
          std::string                                          kty; // Key Type
          std::string                                          x; // "x"
          std::string                                          y; // "y"
          } jwk;                                          //jwk field
        std::string                                     alg; //Key algo
        } header;                                        //header field http://self-issued.info/docs/draft-ietf-jose-json-web-signature.html
      std::string signature;  // Key signature
      std::string protected_; // protected is a keyword :P

    };
    const std::string        mediaType = "application/vnd.docker.distribution.manifest.v1+prettyjws";
  };

  auto operator==( ImageManifest const &im1, ImageManifest const &im2 ) -> bool;
  auto operator!=( ImageManifest const &im1, ImageManifest const &im2 ) -> bool;

  void from_json( const nlohmann::json &j, ImageManifest &im );
  void from_json( const nlohmann::json &j, SignedImageManifest &sim );
  void from_json( const nlohmann::json &j, SignedImageManifest::Signature &sims );

  void to_json( nlohmann::json &j, ImageManifest const &im );
} // namespace OCI::Schema1
