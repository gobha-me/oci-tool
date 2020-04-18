#include <OCI/Registry/Error.hpp>

void OCI::Registry::from_json( const nlohmann::json& j, Error& err ) {
  j.at( "code" ).get_to( err.code );
  j.at( "message" ).get_to( err.message );

  if ( j.find( "detail" ) != j.end() ) {
    err.detail = j.at( "detail" ).dump();
  }
}
