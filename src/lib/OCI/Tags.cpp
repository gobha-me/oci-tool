#include <OCI/Tags.hpp>

void OCI::from_json( const nlohmann::json& j, Tags& t ) {
  j.at( "name" ).get_to( t.name );
  t.tags = j.at( "tags" ).get< std::vector< std::string > >();
}

void OCI::to_json( nlohmann::json& j, const Tags& t ) {
  (void)j;
  (void)t;

  std::cout << "Construst json from OCI::Tags" << std::endl;
}
