#pragma once
#include <OCI/Base/Client.hpp>
#include <OCI/Manifest.hpp>
#include <OCI/Schema1.hpp>
#include <OCI/Schema2.hpp>
#include <string>

namespace OCI {
  void Copy( const std::string& rsrc, const std::string& target, OCI::Base::Client* src, OCI::Base::Client* dest );

  void Copy( const Schema1::ImageManifest& image_manifest, OCI::Base::Client* src, OCI::Base::Client* dest );

  void Copy( const Schema2::ManifestList& manifest_list, OCI::Base::Client* src, OCI::Base::Client* dest );

  void Copy( const Schema2::ImageManifest& image_manifest, OCI::Base::Client* src, OCI::Base::Client* dest );
} // namespace OCI
