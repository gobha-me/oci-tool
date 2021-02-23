#include <OCI/Tags.hpp>
#include <spdlog/spdlog.h>

void OCI::from_json( const nlohmann::json &j, Tags &t ) {
  j.at( "name" ).get_to( t.name );
  t.tags = j.at( "tags" ).get< std::vector< std::string > >();
}

void OCI::to_json( nlohmann::json &j, const Tags &t ) {
  (void)j;
  (void)t;

  spdlog::warn( "Construst json from OCI::Tags" );
}
