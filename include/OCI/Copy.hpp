#pragma once
#include <gobha/SimpleThreadManager.hpp>
#include <OCI/Base/Client.hpp>
#include <OCI/Manifest.hpp>
#include <OCI/Schema1.hpp>
#include <OCI/Schema2.hpp>
#include <indicators.hpp>
#include <memory>
#include <string>

namespace OCI {
  using ProgressBars = indicators::DynamicProgress< indicators::ProgressBar >;

  class Copy {
  public:
    Copy();
    Copy( Copy const& ) = delete;
    Copy( Copy && ) = delete;

    auto operator=( Copy const& ) = delete;
    auto operator=( Copy && ) = delete;

    auto execute( std::string const &rsrc, std::string const &target, OCI::Base::Client *src, OCI::Base::Client *dest ) -> void;
    auto execute( Schema1::ImageManifest const &image_manifest ) -> void;
    auto execute( Schema1::SignedImageManifest const &image_manifest ) -> void;
    auto execute( Schema2::ManifestList &manifest_list ) -> void;
    auto execute( Schema2::ImageManifest const &image_manifest, std::string &target ) -> bool;

    friend class Sync;
  protected:
    static auto getIndicator( size_t max_progress, std::string const& prefix, indicators::Color color = indicators::Color::white ) -> indicators::ProgressBar;
  private:
    std::shared_ptr< gobha::SimpleThreadManager > _stm{nullptr};
    std::shared_ptr< ProgressBars > _progress_bars{nullptr};

    static std::mutex                 _wd_mutex;
    static std::vector< std::string > _working_digests;

    OCI::Base::Client * _src{nullptr};
    OCI::Base::Client * _dest{nullptr};
  };
} // namespace OCI
