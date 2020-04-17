#include <OCI/Schema1.hpp>

void OCI::Schema1::from_json( const nlohmann::json& j, OCI::Schema1::ImageManifest& im ) {
  (void)j;
  (void)im;

  std::cout << "Construct OCI::Schema1::Manifest" << std::endl;
}

void OCI::Schema1::from_json( const nlohmann::json& j, OCI::Schema1::SignedImageManifest& sim ) {
  (void)j;
  (void)sim;

  std::cout << "Construct OCI::Schema1::SignedManifest" << std::endl;
}

void OCI::Schema1::from_json( const nlohmann::json& j, OCI::Schema1::SignedImageManifest::Signature& sims ) {
  (void)j;
  (void)sims;

  std::cout << "Construct OCI::Schema1::SignedManifest::Signature" << std::endl;
}
