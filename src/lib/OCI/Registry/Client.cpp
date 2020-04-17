#include <OCI/Registry/Client.hpp>
constexpr auto SSL_PORT = 443;

enum class HTTP_CODE {
  OK              = 200,
  Created         = 201,
  Accepted        = 202,
  Moved           = 301,
  Found           = 302,
  Temp_Redirect   = 307,
  Perm_Redirect   = 308,
  Bad_Request     = 400,
  Unauthorized    = 401,
  Forbidden       = 403,
  Not_Found       = 404,
  Inter_Srv_Err   = 500,
  Not_Implemented = 501,
  Bad_Gateway     = 502,
  Service_Unavail = 503,
};

OCI::Registry::Client::Client() : _cli( nullptr ) {}
OCI::Registry::Client::Client(  std::string const & domain ) : 
                                _cli( std::make_shared< httplib::SSLClient >( domain, SSL_PORT ) ),
                                _domain( domain ) {
  _cli->set_follow_location( true );
}

OCI::Registry::Client::Client(  std::string const & domain,
                                std::string const & username,
                                std::string const & password ) : 
                                _cli( std::make_shared< httplib::SSLClient >( domain, SSL_PORT ) ),
                                _domain( domain ),
                                _username( username ),
                                _password( password ) {
  _cli->set_follow_location( true );
}

void OCI::Registry::Client::auth( std::string rsrc ) {
  if ( not _username.empty() and not _password.empty() )
    _cli->set_basic_auth( _username.c_str(), _password.c_str() );

  auto res = _cli->Get( std::string( "/v2/" + rsrc ).c_str() );

  if ( res == nullptr ) { // need to handle timeouts a little more gracefully
    std::abort(); // Need to figure out how this API is going to handle errors
    // And I keep adding to the technical debt
  } else if ( HTTP_CODE( res->status ) == HTTP_CODE::Not_Found ) {
    std::cerr << "/v2/" << rsrc << std::endl;
    std::cerr << rsrc << " " << res->body << std::endl;
    std::abort();
  }

  if ( res->status == 302 ) {
    std::cerr << "Auto redirect not enabled: file a bug" << std::endl;
    std::abort();
  }

  if ( HTTP_CODE( res->status ) != HTTP_CODE::OK ) { // This hot mess just to get the auth endpoint
    auto errors     = nlohmann::json::parse( res->body ).at( "errors" ).get< std::vector< Error > >();
    auto www_auth   = res->headers.find( "Www-Authenticate" );

    if ( www_auth != res->headers.end() ) {
      auto auth_type  = www_auth->second.substr( 0, www_auth->second.find( ' ' ) );
      auto auth_hint  = www_auth->second.substr( www_auth->second.find( ' ' ) + 1 );
      auto coma       = auth_hint.find( ',' );
      auto ncoma      = auth_hint.find( ',', coma + 1 );

      //std::cout << "|" << auth_type << "|" << " |" << auth_hint << "|" << std::endl;

      auto realm      = auth_hint.substr( 0, coma );
           realm      = realm.substr( realm.find( '"' ) + 1, realm.length() - ( realm.find( '"' ) + 2 ) );
           realm      = realm.substr( realm.find( '/' ) + 2 ); // remove proto
      auto endpoint   = realm.substr( realm.find( '/' ) );
           realm      = realm.substr( 0, realm.find( '/') );
      auto service    = auth_hint.substr( coma + 1, ncoma - coma - 1 );
           service    = service.substr( service.find( '"' ) + 1, service.length() - ( service.find( '"' ) + 2 ) );
      auto scope      = auth_hint.substr( ncoma + 1 );
           scope      = scope.substr( scope.find( '"' ) + 1, scope.length() - ( scope.find( '"' ) + 2 ) );

      //std::cout << realm << endpoint << "?service=" << service << "&scope=" << scope << std::endl;
      httplib::SSLClient client( realm, SSL_PORT );
      auto result = client.Get( ( endpoint + "?service=" + service + "&scope=" + scope ).c_str() );

      if ( result == nullptr ) {
        std::abort(); // Need to figure out how this API is going to handle errors
        // And I keep adding to the technical debt
      }

      // std::cout << nlohmann::json::parse( result->body ).dump( 2 ) << std::endl;

      auto j = nlohmann::json::parse( result->body );

      if ( j.find( "token" ) == j.end() ) {
        std::cerr << "Auth Failed: " << j.dump( 2 ) << std::endl;
      } else {
        _token = j.at( "token" );
      }
    }
  }
}

httplib::Headers OCI::Registry::Client::defaultHeaders() {
  return {
    { "Authorization", "Bearer " + _token }
  };
}

void OCI::Registry::Client::fetchBlob( const std::string& rsrc, SHA256 sha, std::function< void(const char *, uint64_t ) >& call_back ) {
  (void)rsrc;
  (void)sha;
  (void)call_back;

  std::cout << "OCI::Registry::Client::fetchBlob is not implemented!" << std::endl;
}

bool OCI::Registry::Client::hasBlob( const std::string& rsrc, SHA256 sha ) {
  auto res = _cli->Head( std::string( "/v2/" + rsrc + "/blobs/" + sha ).c_str(), defaultHeaders() );

  if ( HTTP_CODE( res->status ) == HTTP_CODE::Not_Found ) {
    auth( rsrc + "/blobs/" + sha ); // auth modifies the headers, so should auth return headers???

    res = _cli->Get( std::string( "/v2/" + rsrc + "/manifests/" + sha ).c_str(), defaultHeaders() );

    if ( res == nullptr ) {
      std::abort();
    }
  }

  if ( HTTP_CODE( res->status ) != HTTP_CODE::OK ) {
    std::cerr << rsrc << ":" << sha << " Status: " << res->status << " Body: " << res->body << std::endl;
  }

  return HTTP_CODE( res->status ) == HTTP_CODE::OK;
}

void OCI::Registry::Client::inspect( std::string base_rsrc, std::string target ) {
  using namespace std::string_literals;

  auto rsrc   = base_rsrc;
  auto tags   = tagList( rsrc );

  auto manifestList = OCI::Manifest< Schema2::ManifestList >( this, rsrc, target );

  if ( manifestList.schemaVersion == 1 ) { // NOLINT Fall back to Schema1
      auto image_manifest = OCI::Manifest< Schema1::ImageManifest >( this, rsrc, target );
  } else if ( manifestList.schemaVersion == 2 ) { // NOLINT
    // need to do more than this, doing this instead of to_json for the moment
    // want a "combined" struct for this data and a to_json for it so the output can be "pretty printed"
    std::cout << "name: " << manifestList.name << "\n";
    std::cout << "schemaVersion: " << manifestList.schemaVersion << "\n";
    std::cout << "mediaType: " << manifestList.mediaType << "\n";
    std::cout << "tags: [" << "\n";
    for ( auto const& tag: tags.tags )
      std::cout << "  " << tag << "\n";
    std::cout << "]" << "\n";
    std::cout << "manifests: [\n";
    for ( auto const & manifest_item: manifestList.manifests ) {
      std::cout << "  {\n";
      std::cout << "    mediaType: " << manifest_item.mediaType << "\n";
      std::cout << "    size: " << manifest_item.size << "\n";
      std::cout << "    digest: " << manifest_item.digest << "\n";
      std::cout << "    architecture: " << manifest_item.platform.architecture << "\n";
      std::cout << "    os: " << manifest_item.platform.os << "\n";

      if ( not manifest_item.platform.os_version.empty() )
        std::cout << "    os.version: " << manifest_item.platform.os_version << "\n";

      for ( auto const & os_feature: manifest_item.platform.os_features )
        std::cout << "    os.feature: " << os_feature << "\n";

      if ( not manifest_item.platform.variant.empty() )
        std::cout << "    variant: " << manifest_item.platform.variant << "\n";

      for ( auto const & feature: manifest_item.platform.features )
        std::cout << "    feature: " << feature << "\n";

      std::cout << "    ImageManifest: {\n";
      auto image_manifest = OCI::Manifest< Schema2::ImageManifest >( this, rsrc, target );
      std::cout << "      schemaVersion: " << image_manifest.schemaVersion << "\n";
      std::cout << "      mediaType: " << image_manifest.mediaType << "\n";
      std::cout << "      config: { \n";
      std::cout << "        mediaType: " << image_manifest.config.mediaType << "\n";
      std::cout << "        size: " << image_manifest.config.size << "\n";
      std::cout << "        digest: " << image_manifest.config.digest << "\n";
      std::cout << "        layers: {\n";

      for ( auto const& layer: image_manifest.layers ) {
        std::cout << "          mediaType: " << layer.mediaType << "\n";
        std::cout << "          size: " << layer.size << "\n";
        std::cout << "          digest: " << layer.digest << "\n";
        std::cout << "          urls: {\n";
        for ( auto const& url: layer.urls )
          std::cout << "            url: " << url << "\n";
        std::cout << "          }\n";
      }

      std::cout << "        }\n";
      std::cout << "      }\n";
      std::cout << "    }\n";
      std::cout << "  },\n";
    }
    std::cout << "]" << std::endl;
  }
}

void OCI::Registry::Client::manifest( Schema1::ImageManifest& im, const std::string& rsrc, const std::string& target ) {
  auto res = manifest( im.mediaType, rsrc, target );

  if ( HTTP_CODE( res->status ) == HTTP_CODE::OK ) {
    nlohmann::json::parse( res->body ).get_to( im );

    if ( im.name.empty() )
      im.name = rsrc;
  } else {
    std::cerr << res->body << std::endl;
  }
}

void OCI::Registry::Client::manifest( Schema1::SignedImageManifest& sim, const std::string& rsrc, const std::string& target ) {
  auto res = manifest( sim.mediaType, rsrc, target );

  if ( HTTP_CODE( res->status ) == HTTP_CODE::OK ) {
    nlohmann::json::parse( res->body ).get_to( sim );

    if ( sim.name.empty() )
      sim.name = rsrc;
  } else {
    std::cerr << res->body << std::endl;
  }
}

void OCI::Registry::Client::manifest( Schema2::ManifestList& ml, const std::string& rsrc, const std::string& target ) {
  auto res = manifest( ml.mediaType, rsrc, target );

  if ( HTTP_CODE( res->status ) == HTTP_CODE::OK ) {
    nlohmann::json::parse( res->body ).get_to( ml );

    if ( ml.name.empty() )
      ml.name = rsrc;
  } else {
    std::cerr << res->body << std::endl;
  }
}

void OCI::Registry::Client::manifest( Schema2::ImageManifest& im, const std::string& rsrc, const std::string& target ) {
  auto res = manifest( im.mediaType, rsrc, target );

  if ( HTTP_CODE( res->status ) == HTTP_CODE::OK ) {
    try {
      nlohmann::json::parse( res->body ).get_to( im );

      if ( im.name.empty() )
        im.name = rsrc;
    } catch ( nlohmann::detail::out_of_range & err ) {
      std::cerr << "Status: " << res->status << " Body: " << res->body << std::endl;
      std::cerr << err.what() << std::endl;

      throw;
    }
  } else {
    std::cerr << res->body << std::endl;
  }
}

std::shared_ptr< httplib::Response> OCI::Registry::Client::manifest( const std::string& mediaType, const std::string& resource, const std::string& target ) {
  auto headers = defaultHeaders();
  headers.emplace( "Accept", mediaType );

  auto res = _cli->Get( std::string( "/v2/" + resource + "/manifests/" + target ).c_str(), headers );

  if ( res == nullptr ) {
    std::abort();
  }

  if ( HTTP_CODE( res->status ) == HTTP_CODE::Unauthorized ) {
    auth( resource + "/manifests/" + target ); // auth modifies the headers, so should auth return headers???
    headers = defaultHeaders();
    headers.emplace( "Accept", mediaType );

    res = _cli->Get( std::string( "/v2/" + resource + "/manifests/" + target ).c_str(), headers );

    if ( res == nullptr ) {
      std::abort();
    }
  } else if ( HTTP_CODE( res->status ) != HTTP_CODE::OK ) {
    std::cerr << "Err other than unauthorized attempting to get '" << resource << "'! Status: " << res->status << std::endl;
  }

  return res;
}

void OCI::Registry::Client::putBlob( const std::string& rsrc, const std::string& target, std::uintmax_t total_size, const char * blob_part, uint64_t blob_part_size ) {
  (void)rsrc;
  (void)target;
  (void)total_size;
  (void)blob_part;
  (void)blob_part_size;
}

OCI::Tags OCI::Registry::Client::tagList( const std::string& rsrc ) {
  Tags retVal;

  auto res = _cli->Get( std::string( "/v2/" + rsrc + "/tags/list" ).c_str(), defaultHeaders() );

  if ( HTTP_CODE( res->status ) != HTTP_CODE::OK ) {
    auth( rsrc + "/tags/list" );

    res = _cli->Get( std::string( "/v2/" + rsrc + "/tags/list" ).c_str(), defaultHeaders() );

    if ( res == nullptr ) {
      std::abort();
    }
  }

  if ( HTTP_CODE( res->status ) == HTTP_CODE::OK ) {
    retVal = nlohmann::json::parse( res->body ).get< Tags >();
  } else {
    std::cerr << res->body << std::endl;
  }

  return retVal;
}

bool OCI::Registry::Client::pingResource( std::string rsrc ) {
  bool retVal = false;

  if ( _token.empty() ) {
    auto res  = _cli->Get( std::string( "/v2/" + rsrc ).c_str(), defaultHeaders() );
    retVal    = HTTP_CODE( res->status ) == HTTP_CODE::OK;
  }

  return retVal;
}
