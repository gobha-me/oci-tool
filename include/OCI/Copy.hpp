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
  using ProgressBars   = indicators::DynamicProgress< indicators::ProgressBar >;
  using STM_ptr        = std::shared_ptr< gobha::SimpleThreadManager >;
  using PB_ptr         = std::shared_ptr< ProgressBars >;
  using WorkingDigests = std::vector< std::string >;
  using WD_ptr         = std::shared_ptr< WorkingDigests >;

  auto getIndicator( size_t max_progress, std::string const& prefix, indicators::Color color = indicators::Color::white ) -> indicators::ProgressBar;

  class Copy {
  public:
    Copy( OCI::Base::Client * source, OCI::Base::Client * destination );
    Copy( Copy const& ) = delete;
    Copy( Copy && ) = delete;

    ~Copy();

    auto operator=( Copy const& ) = delete;
    auto operator=( Copy && ) = delete;

    auto execute( std::string const &rsrc, std::string const &target ) -> void;
    auto execute( Schema1::ImageManifest const &image_manifest ) -> void;
    auto execute( Schema1::SignedImageManifest const &image_manifest ) -> void;
    auto execute( Schema2::ManifestList &manifest_list ) -> void;
    auto execute( Schema2::ImageManifest const &image_manifest, std::string &target ) -> bool;

    friend class Sync;
  protected:
    Copy();
    Copy( OCI::Base::Client * source, OCI::Base::Client * destination, STM_ptr stm, PB_ptr progress_bars );
  private:
    STM_ptr _stm{nullptr};
    PB_ptr  _progress_bars{nullptr};

    std::mutex _wd_mutex;
    std::condition_variable _finish_download;
    WD_ptr     _working_digests{nullptr};

    OCI::Base::Client * _src{nullptr};
    OCI::Base::Client * _dest{nullptr};
  };
} // namespace OCI
