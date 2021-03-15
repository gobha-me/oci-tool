#include <OCI/Tags.hpp>
#include <spdlog/spdlog.h>

void OCI::from_json( const nlohmann::json &j, Tags &t ) {
  j.at( "name" ).get_to( t.name );
  // FIXME: Should we sort this here? Version order with "latest" at index 0
  t.tags = j.at( "tags" ).get< std::vector< std::string > >();
}

void OCI::to_json( nlohmann::json &j, const Tags &t ) {
  (void)j;
  (void)t;

  spdlog::warn( "Construst json from OCI::Tags not implemented" );
}
