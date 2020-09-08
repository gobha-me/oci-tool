#pragma once
#include <OCI/Base/Client.hpp>
#include <OCI/Manifest.hpp>
#include <OCI/Schema1.hpp>
#include <OCI/Schema2.hpp>
#include <indicators.hpp>
#include <string>

namespace OCI {
  using ProgressBars = indicators::DynamicProgress< indicators::ProgressBar >;

  void Copy( std::string const &rsrc, std::string const &target, OCI::Base::Client *src, OCI::Base::Client *dest,
             ProgressBars &progress_bars );

  void Copy( Schema1::ImageManifest const &image_manifest, OCI::Base::Client *src, OCI::Base::Client *dest,
             ProgressBars &progress_bars );
  void Copy( Schema1::SignedImageManifest const &image_manifest, OCI::Base::Client *src, OCI::Base::Client *dest,
             ProgressBars &progress_bars );

  void Copy( Schema2::ManifestList &manifest_list, OCI::Base::Client *src, OCI::Base::Client *dest,
             ProgressBars &progress_bars );
  auto Copy( Schema2::ImageManifest const &image_manifest, std::string &target, OCI::Base::Client *src,
             OCI::Base::Client *dest, ProgressBars &progress_bars ) -> bool;
} // namespace OCI
