#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <nlohmann/json.hpp> // https://github.com/nlohmann/json

namespace OCI {
  struct Tags {
    std::string                 name;
    std::vector< std::string >  tags;
  };

  void from_json( const nlohmann::json& j, Tags& t );
  void to_json( nlohmann::json& j, const Tags& t );
}

void OCI::from_json( const nlohmann::json& j, Tags& t ) {
  j.at( "name" ).get_to( t.name );
  t.tags = j.at( "tags" ).get< std::vector< std::string > >();
}

void OCI::to_json( nlohmann::json& j, const Tags& t ) {
  (void)j;
  (void)t;

  std::cout << "Construst json from OCI::Tags" << std::endl;
}
