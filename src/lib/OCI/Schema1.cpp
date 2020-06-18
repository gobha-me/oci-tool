#include <OCI/Schema1.hpp>

void OCI::Schema1::from_json( const nlohmann::json& j, OCI::Schema1::ImageManifest& im ) {
  j.at( "architecture" ).get_to( im.architecture );
  j.at( "name" ).get_to( im.name );
  j.at( "tag" ).get_to( im.tag );

  if ( j.find( "fsLayers" ) != j.end() ) {
    for ( auto const& layer : j.at( "fsLayers" ) ) {
      im.fsLayers.emplace_back( std::make_pair( "blobSum", layer.at( "blobSum" ) ) ); 
    }
  }

  if ( j.find( "history" ) != j.end() ) {
    for ( auto const& history : j.at( "history" ) ) {
      for ( auto const& [ key, value ] : history.items() ) {
        im.history.emplace_back( std::make_pair( key, value ) );
      }
    }
  }
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
