#include <OCI/Schema1.hpp>
#include <spdlog/spdlog.h>

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

  if ( j.find( "raw_str" ) != j.end() ) {
    j.at( "raw_str" ).get_to( im.raw_str );
  }
}

void OCI::Schema1::from_json( const nlohmann::json& j, OCI::Schema1::SignedImageManifest& sim ) {
  (void)j;
  (void)sim;

  spdlog::warn( "Construct OCI::Schema1::SignedManifest" );
}

void OCI::Schema1::from_json( const nlohmann::json& j, OCI::Schema1::SignedImageManifest::Signature& sims ) {
  (void)j;
  (void)sims;

  spdlog::warn( "Construct OCI::Schema1::SignedManifest::Signature" );
}

void OCI::Schema1::to_json( nlohmann::json& j, OCI::Schema1::ImageManifest const& im ) {
  j = nlohmann::json{
    { "schemaVersion", im.schemaVersion },
    { "architecture", im.architecture },
    { "name", im.name },
    { "fsLayers", im.fsLayers },
    { "history", im.history },
    { "raw_str", im.raw_str }
  };
}
