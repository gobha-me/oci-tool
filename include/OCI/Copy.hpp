#pragma once
#include <OCI/Base/Client.hpp>
#include <OCI/Manifest.hpp>
#include <OCI/Schema1.hpp>
#include <OCI/Schema2.hpp>
#include <string>

namespace OCI {
  void Copy( const std::string& rsrc, const std::string& target, OCI::Base::Client* src, OCI::Base::Client* dest );

  void Copy( const Schema1::ImageManifest& image_manifest, const std::string& target, OCI::Base::Client* src, OCI::Base::Client* dest );

  void Copy( const Schema2::ManifestList& manifest_list, const std::string& target, OCI::Base::Client* src, OCI::Base::Client* dest );

  auto Copy( const Schema2::ImageManifest& image_manifest, const std::string& target, OCI::Base::Client* src, OCI::Base::Client* dest ) -> bool;
} // namespace OCI
