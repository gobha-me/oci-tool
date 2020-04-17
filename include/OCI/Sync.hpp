#pragma once
#include <OCI/Base/Client.hpp>
#include <OCI/Copy.hpp>

namespace OCI {
  void Sync( const std::string& rsrc, OCI::Base::Client* src, OCI::Base::Client* dest );

  void Sync( const std::string& rsrc, const std::vector< std::string >& tags, OCI::Base::Client* src, OCI::Base::Client* dest );
} // namespace OCI
