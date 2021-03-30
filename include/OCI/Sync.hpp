#pragma once
#include <OCI/Base/Client.hpp>
#include <OCI/Copy.hpp>
#include <OCI/Extensions/Yaml.hpp>

namespace OCI {
  class Sync {
  public:
    Sync();

    void execute( OCI::Extensions::Yaml *src, OCI::Base::Client *dest );
    void execute( OCI::Base::Client *src, OCI::Base::Client *dest );

  protected:
    void execute( std::string const &rsrc );
    void execute( std::string const &rsrc, std::vector< std::string > const &tags );

    void repoSync( OCI::Catalog const &catalog, ProgressBars::BarGuard &sync_bar_ref );

  private:
    std::unique_ptr< Copy >                       copier_{ nullptr };
    std::shared_ptr< gobha::SimpleThreadManager > stm_{ nullptr };
    std::shared_ptr< ProgressBars >               progress_bars_{ nullptr };
  };
} // namespace OCI
