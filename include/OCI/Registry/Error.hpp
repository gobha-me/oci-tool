#pragma once
#include <nlohmann/json.hpp> // https://github.com/nlohmann/json
#include <string>

namespace OCI { // https://docs.docker.com/registry/spec/api/
  namespace Registry {
    struct Error {
      std::string code;
      std::string message;
      std::string detail; // unstructured and optional, store as string unless the data is required to be parsed
    };

    void from_json( const nlohmann::json &j, Error &err );
  } // namespace Registry
} // namespace OCI
