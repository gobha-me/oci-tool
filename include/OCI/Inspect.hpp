#pragma once
#include <OCI/Base/Client.hpp>
#include <OCI/Manifest.hpp>
#include <OCI/Schema1.hpp>
#include <OCI/Schema2.hpp>
#include <string>

namespace OCI {
  void Inspect( std::string const &rsrc, std::string const &target, OCI::Base::Client *client );
} // namespace OCI
