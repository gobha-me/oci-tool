#pragma once
#include <OCI/Base/Client.hpp>
#include <OCI/Manifest.hpp>
#include <OCI/Schema1.hpp>
#include <OCI/Schema2.hpp>
#include <string>

namespace OCI {
  void Copy( std::string const& rsrc, std::string const& target, OCI::Base::Client* src, OCI::Base::Client* dest );

  void Copy( Schema1::ImageManifest const& image_manifest, OCI::Base::Client* src, OCI::Base::Client* dest );
  void Copy( Schema1::SignedImageManifest const& image_manifest, OCI::Base::Client* src, OCI::Base::Client* dest );

  void Copy( Schema2::ManifestList       & manifest_list,  OCI::Base::Client* src, OCI::Base::Client* dest );
  auto Copy( Schema2::ImageManifest const& image_manifest, std::string& target,    OCI::Base::Client* src, OCI::Base::Client* dest ) -> bool;
} // namespace OCI
