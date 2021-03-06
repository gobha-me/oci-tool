#pragma once
#include <OCI/Base/Client.hpp>
#include <OCI/Extensions/Dir.hpp>
#include <OCI/Registry/Client.hpp>
#include <functional>
#include <map>
#include <memory>

namespace OCI {
  const std::map< std::string, std::function< std::unique_ptr< OCI::Base::Client >(
                                   std::string const &, std::string const &, std::string const & ) > >
      CLIENT_MAP = {
          { "dir",
            []( std::string const &base_dir, [[maybe_unused]] std::string const &username,
                [[maybe_unused]] std::string const &password ) {
              return std::make_unique< OCI::Extensions::Dir >( base_dir );
            } },
          { "docker",
            []( std::string const &location, std::string const &username, std::string const &password ) {
              return std::make_unique< OCI::Registry::Client >( location, username, password );
            } },
  };
} // namespace OCI
