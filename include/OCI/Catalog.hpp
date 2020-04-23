#pragma once
#include <string>
#include <vector>

namespace OCI {
  // there is room for local extensions if needed
  struct Catalog {
    std::vector< std::string > repositories;
  };
} // namespace OCI
