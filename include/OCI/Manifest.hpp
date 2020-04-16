#pragma once
#include <string>
#include <OCI/Base/Client.hpp>

namespace OCI {
  template< class Schema_t > // Schema_t is an object that has attributes name and mediaType and an overloaded from_json
  Schema_t Manifest( OCI::Base::Client* client, const std::string& rsrc, const std::string& target );
}


template< class Schema_t >
Schema_t OCI::Manifest( OCI::Base::Client* client, const std::string& rsrc, const std::string& target ) {
  Schema_t retVal;

  client->manifest( retVal, rsrc, target );

  return retVal;
}
