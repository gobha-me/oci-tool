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
    std::unique_ptr< Copy >                       _copier{ nullptr };
    std::shared_ptr< gobha::SimpleThreadManager > _stm{ nullptr };
    std::shared_ptr< ProgressBars >               _progress_bars{ nullptr };
  };
} // namespace OCI
