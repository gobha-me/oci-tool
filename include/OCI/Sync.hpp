#pragma once
#include <OCI/Base/Client.hpp>
#include <OCI/Copy.hpp>
#include <OCI/Extensions/Yaml.hpp>

namespace OCI {
  class Sync {
  public:
    Sync();

    auto execute( OCI::Extensions::Yaml *src, OCI::Base::Client *dest ) -> void;
    auto execute( OCI::Base::Client *src, OCI::Base::Client *dest ) -> void;

  protected:
    auto execute( std::string const &rsrc ) -> void;
    auto execute( std::string const &rsrc, std::vector< std::string > const &tags ) -> void;

  private:
    std::unique_ptr< Copy >                       copier_{ nullptr };
    std::shared_ptr< gobha::SimpleThreadManager > stm_{ nullptr };
    std::shared_ptr< ProgressBars >               progress_bars_{ nullptr };
  };
} // namespace OCI
