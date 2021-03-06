#include <OCI/Schema2.hpp>

auto OCI::Schema2::operator==( ImageManifest const &im1, ImageManifest const &im2 ) -> bool {
  return im1.layers == im2.layers and im1.config.digest == im2.config.digest;
}

auto OCI::Schema2::operator!=( ImageManifest const &im1, ImageManifest const &im2 ) -> bool {
  return not( im1 == im2 );
}

auto OCI::Schema2::operator==( ImageManifest::Layer const &iml1, ImageManifest::Layer const &iml2 ) -> bool {
  return iml1.digest == iml2.digest;
}

auto OCI::Schema2::operator!=( ImageManifest::Layer const &iml1, ImageManifest::Layer const &iml2 ) -> bool {
  return not( iml1 == iml2 );
}

auto OCI::Schema2::operator==( ManifestList const &ml1, ManifestList const &ml2 ) -> bool {
  //  bool retVal = true;
  auto retVal = ml1.raw_str == ml2.raw_str;

  //  if ( ml1.name == ml2.name ) {
  //    for ( auto const& m: ml1.manifests ) {
  //      auto ml2_m_itr = std::find_if( ml2.manifests.begin(), ml2.manifests.end(), [m]( ManifestList::Manifest const&
  //      x ) {
  //          return m.digest == x.digest;
  //        } );
  //
  //      if ( ml2_m_itr == ml2.manifests.end() ) {
  //        retVal = false;
  //        break;
  //      }
  //    }
  //  } else {
  //    retVal = false;
  //  }

  return retVal;
}

auto OCI::Schema2::operator!=( ManifestList const &ml1, ManifestList const &ml2 ) -> bool { return not( ml1 == ml2 ); }

void OCI::Schema2::from_json( const nlohmann::json &j, ManifestList &ml ) {
  j.at( "schemaVersion" ).get_to( ml.schemaVersion );

  if ( j.find( "raw_str" ) != j.end() ) {
    j.at( "raw_str" ).get_to( ml.raw_str );
  }

  if ( j.find( "manifests" ) != j.end() ) {
    ml.manifests = j.at( "manifests" ).get< std::vector< ManifestList::Manifest > >();
  }
}

void OCI::Schema2::from_json( const nlohmann::json &j, ManifestList::Manifest &mlm ) {
  j.at( "mediaType" ).get_to( mlm.mediaType );
  j.at( "size" ).get_to( mlm.size );
  j.at( "digest" ).get_to( mlm.digest );
  j.at( "platform" ).at( "architecture" ).get_to( mlm.platform.architecture );
  j.at( "platform" ).at( "os" ).get_to( mlm.platform.os );

  if ( j.at( "platform" ).find( "os.version" ) != j.at( "platform" ).end() ) {
    j.at( "platform" ).at( "os.version" ).get_to( mlm.platform.os_version );
  }

  if ( j.at( "platform" ).find( "os.features" ) != j.at( "platform" ).end() ) {
    j.at( "platform" ).at( "os.features" ).get_to( mlm.platform.os_features );
  }

  if ( j.at( "platform" ).find( "variant" ) != j.at( "platform" ).end() ) {
    j.at( "platform" ).at( "variant" ).get_to( mlm.platform.variant );
  }

  if ( j.at( "platform" ).find( "features" ) != j.at( "platform" ).end() ) {
    j.at( "platform" ).at( "features" ).get_to( mlm.platform.features );
  }
}

void OCI::Schema2::to_json( nlohmann::json &j, const ManifestList &ml ) {
  j = nlohmann::json{
      { "schemaVersion", ml.schemaVersion }, { "mediaType", ml.mediaType }, { "manifests", ml.manifests } };

  nlohmann::json jm = ml.manifests;

  j[ "manifests" ] = jm;
  j[ "raw_str" ]   = ml.raw_str;
}

void OCI::Schema2::to_json( nlohmann::json &j, const ManifestList::Manifest &mlm ) {
  j                                 = nlohmann::json( {} );
  j[ "mediaType" ]                  = mlm.mediaType;
  j[ "size" ]                       = mlm.size;
  j[ "digest" ]                     = mlm.digest;
  j[ "platform" ]                   = nlohmann::json( {} );
  j[ "platform" ][ "architecture" ] = mlm.platform.architecture;
  j[ "platform" ][ "os" ]           = mlm.platform.os;

  if ( not mlm.platform.os_version.empty() ) {
    j[ "platform" ][ "os.version" ] = mlm.platform.os_version;
  }

  if ( not mlm.platform.os_version.empty() ) {
    j[ "platform" ][ "os.features" ] = mlm.platform.os_features;
  }

  if ( not mlm.platform.variant.empty() ) {
    j[ "platform" ][ "variant" ] = mlm.platform.variant;
  }

  if ( not mlm.platform.features.empty() ) {
    j[ "platform" ][ "features" ] = mlm.platform.features;
  }
}

void OCI::Schema2::from_json( const nlohmann::json &j, ImageManifest &im ) {
  if ( j.find( "schemaVersion" ) != j.end() ) {
    j.at( "schemaVersion" ).get_to( im.schemaVersion );

    if ( j.find( "mediaType" ) != j.end() ) {
      j.at( "mediaType" ).get_to( im.mediaType );
    }

    if ( j.find( "config" ) != j.end() ) {
      j.at( "config" ).at( "mediaType" ).get_to( im.config.mediaType );
      j.at( "config" ).at( "size" ).get_to( im.config.size );
      j.at( "config" ).at( "digest" ).get_to( im.config.digest );
    }

    if ( j.find( "layers" ) != j.end() ) {
      im.layers = j.at( "layers" ).get< std::vector< ImageManifest::Layer > >();
    }

    if ( j.find( "raw_str" ) != j.end() ) {
      j.at( "raw_str" ).get_to( im.raw_str );
    }
  }
}

void OCI::Schema2::from_json( const nlohmann::json &j, ImageManifest::Layer &iml ) {
  j.at( "mediaType" ).get_to( iml.mediaType );
  j.at( "size" ).get_to( iml.size );
  j.at( "digest" ).get_to( iml.digest );

  if ( j.find( "urls" ) != j.end() ) {
    j.at( "urls" ).get_to( iml.digest );
  }
}

void OCI::Schema2::to_json( nlohmann::json &j, const ImageManifest &im ) {
  // clang-format off
  j = nlohmann::json{
      { "schemaVersion", im.schemaVersion },
      { "mediaType", im.mediaType },
      { "config",
        {
          { "mediaType", im.config.mediaType },
          { "size", im.config.size },
          { "digest", im.config.digest }
        }
      },
      { "layers", im.layers },
      { "raw_str", im.raw_str }
  };
  // clang-format on
}

void OCI::Schema2::to_json( nlohmann::json &j, const ImageManifest::Layer &iml ) {
  j = nlohmann::json{ { "mediaType", iml.mediaType }, { "size", iml.size }, { "digest", iml.digest } };

  if ( not iml.urls.empty() ) {
    j[ "urls" ] = iml.urls;
  }
}
