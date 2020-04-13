#pragma once
#include <string>
#include <nlohmann/json.hpp> // https://github.com/nlohmann/json

namespace OCI { // https://docs.docker.com/registry/spec/api/
  namespace Registry {
    struct Error {
      std::string code;
      std::string message;
      std::string detail; // unstructured and optional, store as string unless the data is required to be parsed
    };

    void from_json( const nlohmann::json& j, Error& err ); 
  } // namespace Registry
} // namespace OCI

// IMPLEMENTATION
// OCI::Registry::Error
void OCI::Registry::from_json( const nlohmann::json& j, Error& err ) {
  j.at( "code" ).get_to( err.code );
  j.at( "message" ).get_to( err.message );

  if ( j.find( "detail" ) != j.end() )
    err.detail = j.at( "detail" ).dump();
}
