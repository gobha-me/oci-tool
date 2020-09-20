#include <OCI/Registry/Client.hpp>
#include <botan/hash.h>
#include <botan/hex.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <sstream>

constexpr std::uint16_t SSL_PORT    = 443;
constexpr std::uint16_t HTTP_PORT   = 80;
constexpr std::uint16_t DOCKER_PORT = 5000;

enum class HTTP_CODE {
  OK                = 200,
  Created           = 201,
  Accepted          = 202,
  No_Content        = 204,
  Moved             = 301,
  Found             = 302,
  Temp_Redirect     = 307,
  Perm_Redirect     = 308,
  Bad_Request       = 400,
  Unauthorized      = 401,
  Forbidden         = 403,
  Not_Found         = 404,
  Too_Many_Requests = 429,
  Inter_Srv_Err     = 500,
  Not_Implemented   = 501,
  Bad_Gateway       = 502,
  Service_Unavail   = 503,
};

template< class HTTP_CLIENT >
class ToggleLocationGuard {
  public:
    ToggleLocationGuard( HTTP_CLIENT client, bool follow ) : _client( client), _follow( follow ) { // NOLINT
      _client->set_follow_location( _follow );
    }

    ToggleLocationGuard( ToggleLocationGuard const & ) = delete;
    ToggleLocationGuard( ToggleLocationGuard && ) = delete;
    
    ~ToggleLocationGuard() {
      _client->set_follow_location( not _follow );
    }

    auto operator=( ToggleLocationGuard const & ) -> ToggleLocationGuard& = delete;
    auto operator=( ToggleLocationGuard && ) -> ToggleLocationGuard& = delete;

  private:
    HTTP_CLIENT _client;
    bool        _follow;
};

auto splitLocation( std::string location ) -> std::tuple< std::string, std::string, std::string > {
  std::string proto;
  std::string domain;
  std::string uri;

  proto    = location.substr( 0, location.find( ':' ) );  // http or https
  location = location.substr( location.find( '/' ) + 2 ); // strip http:// or https://
  domain   = location.substr( 0, location.find( '/' ) );
  uri      = location.substr( location.find( '/' ) );

  return { proto, domain, uri };
}

OCI::Registry::Client::Client() : _cli( nullptr ), _patch_cli( nullptr ) {}
OCI::Registry::Client::Client( std::string const &location ) {
  auto resource = location;

  if ( resource.find( "//" ) != std::string::npos ) {
    resource = resource.substr( 2 ); // strip the // if exists
  }

  if ( resource.find( '/' ) != std::string::npos ) {
    _domain = resource.substr( 0, resource.find( '/' ) );
  } else {
    _domain = resource;
  }

  // in uri docker will translate to https
  // if docker.io use registry-1.docker.io as the site doesn't redirect
  if ( _domain == "docker.io" ) {
    // either docker.io should work, or they should have a proper redirect implemented
    _domain = "registry-1.docker.io";
  }

  _cli = std::make_shared< httplib::SSLClient >( _domain, SSL_PORT );

  if ( not ping() ) {
    _cli = std::make_shared< httplib::Client >( _domain, DOCKER_PORT );

    if ( not ping() ) {
      spdlog::warn( "{} does not respond to the V2 API (secure/unsecure)", _domain );
      std::terminate();
    }
  }

  _patch_cli = nullptr;
  _cli->set_follow_location( true );
}

OCI::Registry::Client::Client( std::string const &location, std::string username, std::string password )
    : Client( location ) {
  _username = std::move( username );
  _password = std::move( password );
}

OCI::Registry::Client::Client( Client const &other ) {
  _domain   = other._domain;
  _username = other._username;
  _password = other._password;

  _cli = std::make_shared< httplib::SSLClient >( _domain, SSL_PORT );

  if ( not ping() ) {
    _cli = std::make_shared< httplib::Client >( _domain, DOCKER_PORT );
  }

  _cli->set_follow_location( true );
}

OCI::Registry::Client::Client( Client &&other ) noexcept {
  _domain   = std::move( other._domain );
  _username = std::move( other._username );
  _password = std::move( other._password );
  _ctr      = std::move( other._ctr );
  _cli      = std::move( other._cli );
}

auto OCI::Registry::Client::operator=( Client const &other ) -> Client & {
  Client( other ).swap( *this );

  return *this;
}

auto OCI::Registry::Client::operator=( Client &&other ) noexcept -> Client & {
  _domain   = std::move( other._domain );
  _username = std::move( other._username );
  _password = std::move( other._password );
  _ctr      = std::move( other._ctr );
  _cli      = std::move( other._cli );

  return *this;
}

// registry.redhat.io does not provide the scope in the Header, so have to generate it and
//   pass it in as an argument
void OCI::Registry::Client::auth( httplib::Headers const &headers, std::string const &scope ) {
  auto www_auth = headers.find( "Www-Authenticate" );

  if ( www_auth != headers.end() ) {
    auto auth_type = www_auth->second.substr( 0, www_auth->second.find( ' ' ) );
    auto auth_hint = www_auth->second.substr( www_auth->second.find( ' ' ) + 1 );
    auto coma      = auth_hint.find( ',' );
    auto ncoma     = auth_hint.find( ',', coma + 1 );

    std::string location;
    std::string proto;
    std::string domain;

    auto realm    = auth_hint.substr( 0, coma );
    realm         = realm.substr( realm.find( '"' ) + 1, realm.length() - ( realm.find( '"' ) + 2 ) );
    auto service  = auth_hint.substr( coma + 1, ncoma - coma - 1 );
    service       = service.substr( service.find( '"' ) + 1, service.length() - ( service.find( '"' ) + 2 ) );

    std::tie( proto, domain, location ) = splitLocation( realm );

    std::shared_ptr< httplib::Client > client;

    if ( proto == "https" ) {
      client = std::make_shared< httplib::SSLClient >( domain, SSL_PORT );
    } else {
      client = std::make_shared< httplib::SSLClient >( domain, DOCKER_PORT );
    }

    if ( not _username.empty() and not _password.empty() ) {
      client->set_basic_auth( _username.c_str(), _password.c_str() );
    }

    location += "?service=" + service + "&scope=" + scope;

    auto result = client->Get( location.c_str() );

    switch( HTTP_CODE( result->status ) ) {
    case HTTP_CODE::OK: {
        auto j = nlohmann::json::parse( result->body );

        if ( j.find( "token" ) == j.end() ) {
          spdlog::error( "Auth Failed: {}", j.dump( 2 ) );
        } else {
          j.get_to( _ctr );
        }
      }
      break;
    default:
      spdlog::error( "Auth status: {} Location: {} Body: {}", result->status, location, result->body );
      break;
    }
  } else {
    spdlog::error( "OCI::Registry::Client::auth not given header 'Www-Authenticate'" );

    for ( auto const &header : headers ) {
      spdlog::error( "{} -> {}", header.first, header.second );
    }
  }
}

auto OCI::Registry::Client::authHeaders() const -> httplib::Headers {
  httplib::Headers retVal{};

  if ( _ctr.token.empty() or ( _ctr.issued_at + _ctr.expires_in ) >= std::chrono::system_clock::now() ) {
  } else {
    retVal = httplib::Headers{
        { "Authorization", "Bearer " + _ctr.token } // only return this if token is valid
    };
  }

  return retVal;
}

auto OCI::Registry::Client::catalog() -> OCI::Catalog {
  Catalog retVal;

  spdlog::warn( "OCI::Registry::Client::catalog Not implemented" );

  return retVal;
}

auto OCI::Registry::Client::copy() -> std::unique_ptr< OCI::Base::Client > {
  return std::make_unique< OCI::Registry::Client >( *this );
}

auto OCI::Registry::Client::fetchBlob( const std::string &rsrc, SHA256 sha,
                                       std::function< bool( const char *, uint64_t ) > &call_back ) -> bool {
  ToggleLocationGuard< decltype( _cli ) > tlg{ _cli, false };
  bool retVal = true;
  auto client = _cli;

  auto location = std::string( "/v2/" + rsrc + "/blobs/" + sha );
  auto res      = client->Head( location.c_str(), authHeaders() );

  if ( res == nullptr ) {
    spdlog::error( "OCI::Registry::Client::fetchBlob failed to get redirect for {}:{}, returned NULL", rsrc, sha );
    return false;
  }

  switch ( HTTP_CODE( res->status ) ) {
  case HTTP_CODE::Unauthorized:
  case HTTP_CODE::Forbidden:
    auth( res->headers, "repository:" + rsrc + ":pull" );

    res = client->Head( location.c_str(), authHeaders() );
    break;
  default:
    break;
  }

  if ( res->has_header( "Location" ) ) {
    std::string proto;
    std::string domain;

    std::tie( proto, domain, location ) = splitLocation( res->get_header_value( "Location" ) );

    if ( domain != _domain ) {
      if ( proto == "https" ) {
        client = std::make_shared< httplib::SSLClient >( domain, SSL_PORT );
      } else {
        client = std::make_shared< httplib::Client >( domain, DOCKER_PORT );
      }
    }
  }

  auto retries = 0;
  do {
    res = client->Get( location.c_str(), authHeaders(), call_back );
  } while ( res == nullptr and ++retries != 3 );

  if ( res == nullptr ) {
    retVal = false;
    spdlog::error( "OCI::Registry::Client::fetchBlob {}\n client timeout (returned NULL)", location );
  } else {
    switch ( HTTP_CODE( res->status ) ) {
    case HTTP_CODE::OK:
    case HTTP_CODE::Found:
      break;
    default:
      retVal = false;
      spdlog::error( "OCI::Registry::Client::fetchBlob {}\n  Status: {} Body: {}", location, res->status, res->body );
    }
  }

  return retVal;
}

auto OCI::Registry::Client::hasBlob( const Schema1::ImageManifest &im, SHA256 sha ) -> bool {
  ToggleLocationGuard< decltype( _cli ) > tlg{ _cli, false };

  auto client = _cli;

  auto location = "/v2/" + im.name + "/blobs/" + sha;
  auto res      = _cli->Head( location.c_str(), authHeaders() );

  if ( HTTP_CODE( res->status ) == HTTP_CODE::Unauthorized ) {
    auth( res->headers,
          "repository:" + im.name + ":pull" ); // auth modifies the headers, so should auth return headers???

    res = client->Head( location.c_str(), authHeaders() );
  }

  if ( res->has_header( "Location" ) ) {
    std::string proto;
    std::string domain;

    std::tie( proto, domain, location ) = splitLocation( res->get_header_value( "Location" ) );

    if ( domain != _domain ) {
      if ( proto == "https" ) {
        client = std::make_shared< httplib::SSLClient >( domain, SSL_PORT );
      } else {
        client = std::make_shared< httplib::Client >( domain, DOCKER_PORT );
      }
    }

    res = client->Head( location.c_str(), authHeaders() );
  }

  if ( not( HTTP_CODE( res->status ) == HTTP_CODE::OK or HTTP_CODE( res->status ) == HTTP_CODE::Found ) ) {
    spdlog::error( "OCI::Registry::Client::hasBlob {}\n  Status: {} Body: {}", location, res->status, res->body );
    for ( auto const &header : res->headers ) {
      spdlog::error( "{} -> {}", header.first, header.second );
    }
  }

  return HTTP_CODE( res->status ) == HTTP_CODE::OK or HTTP_CODE( res->status ) == HTTP_CODE::Found;
}

auto OCI::Registry::Client::hasBlob( const Schema2::ImageManifest &im, const std::string &target, SHA256 sha ) -> bool {
  (void)target;
  ToggleLocationGuard< decltype( _cli ) > tlg{ _cli, false };

  auto client   = _cli;
  auto location = "/v2/" + im.name + "/blobs/" + sha;
  auto res      = client->Head( location.c_str(), authHeaders() );

  if ( HTTP_CODE( res->status ) == HTTP_CODE::Unauthorized ) {
    auth( res->headers, "repository:" + im.name + ":pull" );

    res = client->Head( location.c_str(), authHeaders() );
  }

  if ( res->has_header( "Location" ) ) {
    std::string proto;
    std::string domain;

    std::tie( proto, domain, location ) = splitLocation( res->get_header_value( "Location" ) );

    if ( domain != _domain ) {
      if ( proto == "https" ) {
        client = std::make_shared< httplib::SSLClient >( domain, SSL_PORT );
      } else {
        client = std::make_shared< httplib::Client >( domain, DOCKER_PORT );
      }
    }
  }

  res = client->Head( location.c_str(), authHeaders() );

  switch ( HTTP_CODE( res->status ) ) {
  case HTTP_CODE::OK:
  case HTTP_CODE::Found:
  case HTTP_CODE::Not_Found: // Common if not yet pushed images so it created a lot of noise
    break;
  default:
    spdlog::error( "OCI::Registry::Client::hasBlob {}\n  Status: {} Body: {}", location, res->status, res->body );
    for ( auto const &header : res->headers ) {
      spdlog::error( "{} -> {}", header.first, header.second );
    }
  }

  return HTTP_CODE( res->status ) == HTTP_CODE::OK or HTTP_CODE( res->status ) == HTTP_CODE::Found;
}

auto OCI::Registry::Client::putBlob( const Schema1::ImageManifest &im, const std::string &target,
                                     std::uintmax_t total_size, const char *blob_part, uint64_t blob_part_size )
    -> bool {
  (void)im;
  (void)target;
  (void)total_size;
  (void)blob_part;
  (void)blob_part_size;

  spdlog::warn( "OCI::Registry::Client::putBlob Schema1::ImageManifest Not_Implemented" );

  return false;
}

auto OCI::Registry::Client::putBlob( Schema2::ImageManifest const &im, std::string const &target,
                                     SHA256 const &blob_sha, std::uintmax_t total_size, const char *blob_part,
                                     uint64_t blob_part_size ) -> bool {
  bool retVal = false;

  // https://docs.docker.com/registry/spec/api/#initiate-blob-upload -- Resumable
  auto                                 headers = authHeaders();
  std::shared_ptr< httplib::Response > res;

  bool          has_error  = false;
  bool          chunk_sent = false;
  std::uint16_t port       = 0;
  std::string   proto;
  std::string   domain;

  if ( _patch_location.empty() ) {
    spdlog::info( "Starting upload for Blob {}", blob_sha );
    res = _cli->Post( ( "/v2/" + im.name + "/blobs/uploads/" ).c_str(), headers, "", "" );
    // FIXME: IF NULL HERE JUST RETRY
  } else {
    headers.emplace( "Host", _domain );
    res = _patch_cli->Get( ( _patch_location ).c_str(), headers );
    // WITHOUT A RESULT AND NEXT ENDPOINT, RESUMING MAY NOT BE FEASIBLE
    // Should we just ensure there is a longer timeout
  }

  while ( res != nullptr and not retVal and not has_error ) {
    switch ( HTTP_CODE( res->status ) ) {
    case HTTP_CODE::Created:
      // The API doc says to expect a 204 No Content as the responce to the PUT
      //   this does make more sense, I hope this is how all the registries respond
      retVal = true;

      break;
    case HTTP_CODE::Unauthorized:
      if ( _auth_retry ) {
        _auth_retry = false;

        auth( res->headers, "repository:" + im.name + ":push" );

        retVal = putBlob( im, target, blob_sha, total_size, blob_part, blob_part_size );
      } else {
        _auth_retry = true;
      }

      break;
    case HTTP_CODE::Accepted: {
      std::tie( proto, domain, _patch_location ) = splitLocation( res->get_header_value( "Location" ) );

      if ( _patch_cli == nullptr ) {
        if ( proto == "https" ) {
          port = SSL_PORT;
        } else {
          port = HTTP_PORT;
        }

        auto has_alt_port = domain.find( ':' );

        if ( has_alt_port != std::string::npos ) {
          port   = std::stoul( domain.substr( has_alt_port + 1 ) );
          domain = domain.substr( 0, has_alt_port );
        }

        if ( proto == "https" ) {
          _patch_cli = std::make_unique< httplib::SSLClient >( domain, port );
        } else {
          _patch_cli = std::make_unique< httplib::Client >( domain, port );
        }
      }
    }

      if ( chunk_sent ) {
        retVal = true;
      } else {
        headers.emplace( "Host", _domain );

        res = _patch_cli->Get( ( _patch_location ).c_str(), headers );
      }

      break;
    case HTTP_CODE::No_Content: {
      if ( chunk_sent ) {
        has_error = true;
      } else {
        chunk_sent       = true;
        auto range       = res->get_header_value( "Range" );
        auto last_offset = std::stoul( range.substr( range.find( '-' ) + 1 ) ) + 1;

        headers.emplace( "Content-Range",
                         std::to_string( last_offset ) + "-" + std::to_string( last_offset + blob_part_size ) );
        headers.emplace( "Content-Length", std::to_string( blob_part_size ) );

        // Not documented, but is part of the API
        httplib::ContentProvider content_provider = [ & ]( size_t offset, size_t length, httplib::DataSink &sink ) {
          (void)offset;
          (void)length;
          sink.write( blob_part, blob_part_size );
        };

        res = _patch_cli->Patch( _patch_location.c_str(), headers, blob_part_size, content_provider,
                                 "application/octet-stream" );

        if ( last_offset + blob_part_size == total_size or blob_part_size == total_size ) {
          // FIXME: need to add attribute to the class to hold bytes_sent for blob count and comparison, so this could
          // can resume an upload, if the application stops for some reason
          //        will also allow the tie below to go away as this block can move to the chunk_sent condition and
          //        finalize there
          std::tie( proto, domain, _patch_location ) = splitLocation( res->get_header_value( "Location" ) );

          headers = authHeaders();
          headers.emplace( "Content-Length", "0" );
          headers.emplace( "Content-Type", "application/octet-stream" );
          // Finalize and label with the digest
          spdlog::info( "Finalizing upload for Blob {}", blob_sha );

          res = _patch_cli->Put( ( _patch_location + "&digest=" + blob_sha ).c_str(), headers, "",
                                 "application/octet-stream" );

          _patch_cli      = nullptr;
          _patch_location = "";
        }
      }
    }

    break;
    default:
      spdlog::error( "OCI::Registry::Client::putBlob  {}\n  Status: {}", im.name, res->status );
      for ( auto const &header : res->headers ) {
        spdlog::error( "{} -> {}", header.first, header.second );
      }
      spdlog::error( " Body: {}", res->body );
      has_error = true;
    }
  }

  if ( res == nullptr ) {
    spdlog::warn( "lost or timed out on our connection to the registry" );
    retVal = false;
  }

  return retVal;
}

void OCI::Registry::Client::fetchManifest( Schema1::ImageManifest &im, Schema1::ImageManifest const &request ) {
  auto json_body = fetchManifest( im.mediaType, request.name, request.requestedTarget );

  json_body.get_to( im );

  if ( im.name.empty() ) {
    im.name = request.name;
  }

  im.originDomain = request.originDomain; // This is just for sync from a Registry to a Directory
}

void OCI::Registry::Client::fetchManifest( Schema1::SignedImageManifest &      sim,
                                           Schema1::SignedImageManifest const &request ) {
  auto json_body = fetchManifest( sim.mediaType, request.name, request.requestedTarget );

  json_body.get_to( sim );

  if ( sim.name.empty() ) {
    sim.name = request.name;
  }

  sim.originDomain = request.originDomain; // This is just for sync from a Registry to a Directory
}

void OCI::Registry::Client::fetchManifest( Schema2::ManifestList &ml, Schema2::ManifestList const &request ) {
  auto json_body = fetchManifest( ml.mediaType, request.name, request.requestedTarget );

  if ( not json_body.empty() ) {
    json_body.get_to( ml );

    if ( ml.name.empty() ) {
      ml.name = request.name;
    }

    if ( request.originDomain.empty() ) {
      ml.originDomain = _domain;
    } else {
      ml.originDomain = request.originDomain;
    }

    ml.requestedTarget = request.requestedTarget;
  }
}

void OCI::Registry::Client::fetchManifest( Schema2::ImageManifest &im, Schema2::ImageManifest const &request ) {
  auto json_body = fetchManifest( im.mediaType, request.name, request.requestedDigest );

  json_body.get_to( im );

  if ( im.name.empty() ) {
    im.name = request.name;
  }

  if ( request.originDomain.empty() ) {
    im.originDomain = _domain;
  } else {
    im.originDomain = request.originDomain; // This is just for sync from a Registry to a Directory
  }

  im.requestedTarget = request.requestedTarget;
  im.requestedDigest = request.requestedDigest;
}

auto OCI::Registry::Client::fetchManifest( const std::string &mediaType, const std::string &resource,
                                           const std::string &target ) -> nlohmann::json {
  auto location = "/v2/" + resource + "/manifests/" + target;
  auto headers  = authHeaders();

  nlohmann::json retVal;

  headers.emplace( "Accept", mediaType );

  auto res = _cli->Get( location.c_str(), headers );

  if ( res == nullptr ) {
    spdlog::error( "OCI::Registry::Client::fetchManifest request error, returned NULL {}:{}", resource, target );
  } else {
    switch ( HTTP_CODE( res->status ) ) {
    case HTTP_CODE::Unauthorized:
      auth( res->headers,
            "repository:" + resource + ":pull" ); // auth modifies the headers, so should auth return headers???

      retVal = fetchManifest( mediaType, resource, target ); // Hopefully this doesn't spiral into an infinite auth loop
      break;
    case HTTP_CODE::OK:
      retVal = nlohmann::json::parse( res->body );
      break;
    case HTTP_CODE::Not_Found:
      spdlog::warn( "OCI::Registry::Client::fetchManifest request Manifest Not_Found {} {}:{}", mediaType, resource,
                    target );
      break;
    default:
      spdlog::error( "OCI::Registry::Client::fetchManifest {}\n  Status: {} Body: {}", location, res->status,
                     res->body );
    }
  }

  return retVal;
}

auto OCI::Registry::Client::putManifest( const Schema1::ImageManifest &im, const std::string &target ) -> bool {
  bool retVal = false;
  (void)im;
  (void)target;

  spdlog::warn( "OCI::Registry::Client::putManifest Schema1::ImageManifest is not implemented" );

  return retVal;
}

auto OCI::Registry::Client::putManifest( const Schema1::SignedImageManifest &sim, const std::string &target ) -> bool {
  bool retVal = false;
  (void)sim;
  (void)target;

  spdlog::warn( "OCI::Registry::Client::putManifest Schema1::SignedImageManifest is not implemented" );

  return retVal;
}

auto OCI::Registry::Client::putManifest( const Schema2::ManifestList &ml, const std::string &target ) -> bool {
  bool           retVal = false;
  nlohmann::json j( ml );

  auto res =
      _cli->Put( ( "/v2/" + ml.name + "/manifests/" + target ).c_str(), authHeaders(), j.dump(), ml.mediaType.c_str() );

  if ( HTTP_CODE( res->status ) == HTTP_CODE::Unauthorized ) {
    auth( res->headers, "repository:" + ml.name + ":push" );

    res = _cli->Put( ( "/v2/" + ml.name + "/manifests/" + target ).c_str(), authHeaders(), j.dump(),
                     ml.mediaType.c_str() );
  }

  switch ( HTTP_CODE( res->status ) ) {
  case HTTP_CODE::Created:
    retVal = true;
    break;
  default:
    spdlog::error( "OCI::Registry::Client::putManifest Schema2::ImageManifest {}\n  Status: ", ml.name, res->status );
    for ( auto const &header : res->headers ) {
      spdlog::error( "{} -> {}", header.first, header.second );
    }
    spdlog::error( " Body: {}", res->body );
  }

  return retVal;
}

auto OCI::Registry::Client::putManifest( Schema2::ImageManifest const &im, std::string &target ) -> bool {
  bool           retVal = false;
  nlohmann::json j( im );

  auto im_str = j.dump();
  auto im_digest( Botan::HashFunction::create( "SHA-256" ) );
  im_digest->update( im_str );

  target = "sha256:" + Botan::hex_encode( im_digest->final() );
  std::for_each( target.begin(), target.end(), []( char &c ) {
    c = std::tolower( c ); // NOLINT - narrowing warning, but unsigned char for the lambda doesn't build, so which is it
  } );

  auto res =
      _cli->Put( ( "/v2/" + im.name + "/manifests/" + target ).c_str(), authHeaders(), j.dump(), im.mediaType.c_str() );

  if ( HTTP_CODE( res->status ) == HTTP_CODE::Unauthorized ) {
    auth( res->headers, "repository:" + im.name + ":push" );

    res = _cli->Put( ( "/v2/" + im.name + "/manifests/" + target ).c_str(), authHeaders(), j.dump(),
                     im.mediaType.c_str() );
  }

  switch ( HTTP_CODE( res->status ) ) {
  case HTTP_CODE::Created:
    retVal = true;
    break;
  default:
    spdlog::error( "OCI::Registry::Client::putManifest Schema2::ImageManifest {}:{}\n  Status: {}", im.name, target,
                   res->status );
    for ( auto const &header : res->headers ) {
      spdlog::error( "{} -> {}", header.first, header.second );
    }
    spdlog::error( " Body: {}", res->body );
    spdlog::error( j.dump( 2 ) );
  }

  return retVal;
}

auto OCI::Registry::Client::tagList( const std::string &rsrc ) -> OCI::Tags {
  Tags retVal;

  auto location = "/v2/" + rsrc + "/tags/list";
  auto res      = _cli->Get( location.c_str(), authHeaders() );

  if ( HTTP_CODE( res->status ) == HTTP_CODE::Unauthorized ) {
    auth( res->headers, "repository:" + rsrc + ":pull" );

    res = _cli->Get( location.c_str(), authHeaders() );
  }

  if ( HTTP_CODE( res->status ) == HTTP_CODE::OK ) {
    retVal = nlohmann::json::parse( res->body ).get< Tags >();
  } else {
    spdlog::error( "OCI::Registry::Client::tagList {}\n  Status: {} Body: {}", location, res->status, res->body );
  }

  return retVal;
}

auto OCI::Registry::Client::tagList( const std::string &rsrc, std::regex const &re ) -> OCI::Tags {
  auto retVal = tagList( rsrc );

  retVal.tags.erase( std::remove_if( retVal.tags.begin(), retVal.tags.end(),
                                     [ re ]( std::string const &tag ) {
                                       std::smatch m;
                                       std::regex_search( tag, m, re );

                                       return m.empty();
                                     } ),
                     retVal.tags.end() );

  return retVal;
}

auto OCI::Registry::Client::ping() -> bool {
  bool retVal = true;

  auto res = _cli->Get( "/v2/" );

  if ( res == nullptr ) { // Most likely an invalid domain or port
    retVal = false;
  } else {
    if ( res->has_header( "Docker-Distribution-Api-Version" ) ) {
      if ( res->get_header_value( "Docker-Distribution-Api-Version" ) != "registry/2.0" ) {
        retVal = false;
      }
    } else {
      retVal = false;
    }
  }

  return retVal;
}

auto OCI::Registry::Client::pingResource( std::string const &rsrc ) -> bool {
  auto const location = "/v2/" + rsrc;
  auto       res      = _cli->Get( location.c_str(), authHeaders() );

  if ( HTTP_CODE( res->status ) == HTTP_CODE::Unauthorized ) {
    auth( res->headers, "repository:" + rsrc + ":pull" );
    res = _cli->Get( location.c_str(), authHeaders() );
  }

  return HTTP_CODE( res->status ) == HTTP_CODE::OK;
}

auto OCI::Registry::Client::swap( Client &other ) -> void {
  std::swap( _domain, other._domain );
  std::swap( _username, other._username );
  std::swap( _password, other._password );
  std::swap( _cli, other._cli );
}

std::mutex TIME_ZONE_MUTEX;
void OCI::Registry::from_json( nlohmann::json const &j, Client::TokenResponse &ctr ) {
  if ( j.find( "token" ) != j.end() ) {
    j.at( "token" ).get_to( ctr.token );
  }

  if ( ctr.token.empty() and j.find( "access_token" ) != j.end() ) {
    j.at( "access_token" ).get_to( ctr.token );
  }

  if ( j.find( "expires_in" ) != j.end() ) {
    ctr.expires_in = std::chrono::seconds( j.at( "expires_in" ).get< int >() );
  } else {
    ctr.expires_in = std::chrono::seconds( 60 ); // NOLINT default value
  }

  if ( j.find( "issued_at" ) != j.end() ) {
    std::tm           tm = {};
    std::stringstream ss( j.at( "issued_at" ).get< std::string >() );
    std::get_time( &tm, "%Y-%m-%dT%H:%M:%S" ); // EXAMPLE 2020-04-20T11:52:16.177118311Z
    
    std::lock_guard< std::mutex > lg( TIME_ZONE_MUTEX );
    ctr.issued_at = std::chrono::system_clock::from_time_t( std::mktime( &tm ) );
  } else {
    ctr.issued_at = std::chrono::system_clock::now();
  }

  if ( j.find( "refresh_token" ) != j.end() ) {
    j.at( "refresh_token" ).get_to( ctr.refresh_token );
  }
}
