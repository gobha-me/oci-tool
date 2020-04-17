#pragma once
#include <OCI/Base/Client.hpp>
#include <string>

namespace OCI {
  template< class Schema_t > // Schema_t is an object that has attributes name and mediaType and an overloaded from_json
  auto Manifest( OCI::Base::Client* client, const std::string& rsrc, const std::string& target ) -> Schema_t;
} // namespace OCI

template< class Schema_t >
auto OCI::Manifest( OCI::Base::Client* client, const std::string& rsrc, const std::string& target ) -> Schema_t {
  Schema_t retVal;

  client->manifest( retVal, rsrc, target );

  return retVal;
}
