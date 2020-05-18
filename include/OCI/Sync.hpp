#pragma once
#include <OCI/Base/Client.hpp>
#include <OCI/Extensions/Yaml.hpp>
#include <OCI/Copy.hpp>

namespace OCI {
  void Sync( OCI::Extensions::Yaml* src, OCI::Base::Client* dest );

  void Sync( OCI::Base::Client* src, OCI::Base::Client* dest );

  void Sync( std::string const& rsrc, OCI::Base::Client* src, OCI::Base::Client* dest );
                              
  void Sync( std::string const& rsrc, std::vector< std::string > const& tags, OCI::Base::Client* src, OCI::Base::Client* dest );
} // namespace OCI
