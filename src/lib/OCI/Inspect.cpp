#include <OCI/Inspect.hpp>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>

// constexpr auto SCHEMA_V1 = 1;
// constexpr auto SCHEMA_V2 = 2;

void OCI::Inspect( std::string const &rsrc, std::string const &target, OCI::Base::Client *client ) {
  using namespace std::string_literals;
  (void)rsrc;
  (void)target;
  (void)client;

  //  auto tags         = client->tagList( rsrc );
  //  auto manifestList = OCI::Manifest< Schema2::ManifestList >( client, rsrc, target );
  //
  //  if ( manifestList.schemaVersion == SCHEMA_V1 ) { // Fall back to Schema1
  //    auto image_manifest = OCI::Manifest< Schema1::ImageManifest >( client, rsrc, target );
  //  } else if ( manifestList.schemaVersion == SCHEMA_V2 ) {
  //    nlohmann::json output = manifestList;
  //    //merge tags
  //    //merge each imageManifest
  //    std::cout << std::setw( 2 ) << output;
  //  }
}
