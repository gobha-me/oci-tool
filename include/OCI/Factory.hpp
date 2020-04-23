#pragma once
#include <OCI/Base/Client.hpp>
#include <OCI/Extensions/Dir.hpp>
#include <OCI/Registry/Client.hpp>
#include <functional>
#include <map>
#include <memory>

const std::map< std::string,
        std::function<
          std::shared_ptr< OCI::Base::Client >( std::string const&, std::string const&, std::string const& )
        >
      > CLIENT_MAP = {
  { "dir",
    [](std::string const& base_dir, std::string const& = "", std::string const& = "" ) {
      return std::make_shared<OCI::Extensions::Dir>( base_dir );
    } },
  { "docker",
    [](std::string const& location, std::string const& username, std::string const& password ) {
      return std::make_shared< OCI::Registry::Client >( location, username, password );
    } },
};
