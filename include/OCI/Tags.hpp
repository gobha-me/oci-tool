#pragma once
#include <iostream>
#include <nlohmann/json.hpp> // https://github.com/nlohmann/json
#include <string>
#include <vector>

namespace OCI {
  struct Tags {
    std::string                name;
    std::vector< std::string > tags;
  };

  void from_json( const nlohmann::json &j, Tags &t );
  void to_json( nlohmann::json &j, const Tags &t );
} // namespace OCI
