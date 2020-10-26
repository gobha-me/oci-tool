#include <OCI/Registry/Client.hpp>
#include <digestpp/algorithm/sha2.hpp>
#include <memory>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdexcept>

constexpr std::uint16_t SSL_PORT    = 443;
constexpr std::uint16_t HTTP_PORT   = 80;
constexpr std::uint16_t DOCKER_PORT = 5000;

enum class HTTP_CODE {
  OK                              = 200,
  Created                         = 201,
  Accepted                        = 202,
  No_Content                      = 204,
  Moved                           = 301,
  Found                           = 302,
  Temp_Redirect                   = 307,
  Perm_Redirect                   = 308,
  Bad_Request                     = 400,
  Unauthorized                    = 401,
  Forbidden                       = 403,
  Not_Found                       = 404,
  Requested_Range_Not_Satisfiable = 416,
  Too_Many_Requests               = 429,
  Client_Closed_Request           = 499,
  Inter_Srv_Err                   = 500,
  Not_Implemented                 = 501,
  Bad_Gateway                     = 502,
  Service_Unavail                 = 503,
};

template< class HTTP_CLIENT >
class ToggleLocationGuard {
  public:
    ToggleLocationGuard( HTTP_CLIENT client, bool follow ) : client_( client), follow_( follow ) { // NOLINT
      client_->set_follow_location( follow_ );
    }

    ToggleLocationGuard( ToggleLocationGuard const & ) = delete;
    ToggleLocationGuard( ToggleLocationGuard && ) = delete;
    
    ~ToggleLocationGuard() {
      client_->set_follow_location( not follow_ );
    }

    auto operator=( ToggleLocationGuard const & ) -> ToggleLocationGuard& = delete;
    auto operator=( ToggleLocationGuard && ) -> ToggleLocationGuard& = delete;

  private:
    HTTP_CLIENT client_;
    bool        follow_;
};

auto splitLocation( std::string location ) -> std::tuple< std::string, std::string, std::string > {
  std::string proto;
  std::string domain;
  std::string uri;

  if ( location.empty() ) {
    spdlog::error( "splitLocation received invalid location string" );
  } else {
    proto    = location.substr( 0, location.find( ':' ) );  // http or https
    location = location.substr( location.find( '/' ) + 2 ); // strip http:// or https://
    domain   = location.substr( 0, location.find( '/' ) );
    uri      = location.substr( location.find( '/' ) );
  }

  return { proto, domain, uri };
}

void http_logger( const httplib::Request & req, const httplib::Response & resp ) {
  spdlog::trace( "http Request" );
  spdlog::trace( " Method: {}", req.method );
  spdlog::trace( " Path: {}", req.path );
  for ( auto const &header : req.headers ) {
    spdlog::trace( " {} -> {}", header.first, header.second );
  }

  spdlog::trace( "http Response Status: {} Version: ", resp.status, resp.version );
  for ( auto const &header : resp.headers ) {
    spdlog::trace( " {} -> {}", header.first, header.second );
  }
  spdlog::trace( resp.body );
}

OCI::Registry::Client::Client() = default;
OCI::Registry::Client::Client( std::string const &location ) {
  auto resource = location;

  if ( resource.find( "//" ) != std::string::npos ) {
    resource = resource.substr( 2 ); // strip the // if exists
  }

  if ( resource.find( '/' ) != std::string::npos ) {
    domain_ = resource.substr( 0, resource.find( '/' ) );
  } else {
    domain_ = resource;
  }

  // in uri docker will translate to https
  // if docker.io use registry-1.docker.io as the site doesn't redirect
  if ( domain_ == "docker.io" ) {
    // either docker.io should work, or they should have a proper redirect implemented
    domain_ = "registry-1.docker.io";
  }

  cli_ = std::make_shared< httplib::SSLClient >( domain_, SSL_PORT );

  if ( not ping() ) {
    cli_ = std::make_shared< httplib::Client >( domain_, DOCKER_PORT );

    if ( not ping() ) {
      std::runtime_error( domain_ + " does not respond to the V2 API (secure/unsecure)" );
    }
  } else {
    secure_con_ = true;
  }

  cli_->set_follow_location( true );
  cli_->set_logger( &http_logger );
}

OCI::Registry::Client::Client( std::string const &location, std::string username, std::string password )
    : Client( location ) {
  username_ = std::move( username );
  password_ = std::move( password );
}

OCI::Registry::Client::Client( Client const &other ) {
  domain_     = other.domain_;
  username_   = other.username_;
  password_   = other.password_;
  secure_con_ = other.secure_con_;

  if ( secure_con_ ) {
    cli_ = std::make_shared< httplib::SSLClient >( domain_, SSL_PORT );
  } else {
    cli_ = std::make_shared< httplib::Client >( domain_, DOCKER_PORT );
  }

  cli_->set_follow_location( true );
  cli_->set_logger( &http_logger );
}

OCI::Registry::Client::Client( Client &&other ) noexcept {
  domain_   = std::move( other.domain_ );
  username_ = std::move( other.username_ );
  password_ = std::move( other.password_ );
  ctr_      = std::move( other.ctr_ );
  cli_      = std::move( other.cli_ );
}

auto OCI::Registry::Client::operator=( Client const &other ) -> Client & {
  Client( other ).swap( *this );

  return *this;
}

auto OCI::Registry::Client::operator=( Client &&other ) noexcept -> Client & {
  domain_   = std::move( other.domain_ );
  username_ = std::move( other.username_ );
  password_ = std::move( other.password_ );
  ctr_      = std::move( other.ctr_ );
  cli_      = std::move( other.cli_ );

  return *this;
}

// registry.redhat.io does not provide the scope in the Header, so have to generate it and
//   pass it in as an argument
void OCI::Registry::Client::auth( httplib::Headers const &headers, std::string const &scope ) {
  auto www_auth = headers.find( "Www-Authenticate" );

  if ( www_auth == headers.end() ) {
    spdlog::error( "OCI::Registry::Client::auth not given header 'Www-Authenticate'" );
  } else {
    spdlog::debug( "OCI::Registry::Client::auth attempt {}", scope );

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

    client->set_logger( &http_logger );

    if ( not username_.empty() and not password_.empty() ) {
      client->set_basic_auth( username_.c_str(), password_.c_str() );
    }

    location += "?service=" + service + "&scope=" + scope;

    auto result = client->Get( location.c_str() );

    if ( result == nullptr ) {
      throw std::runtime_error( "OCI::Registry::Client::auth recieved NULL '" + location + "'" );
    } else {
      switch( HTTP_CODE( result->status ) ) {
      case HTTP_CODE::OK: {
          auto j = nlohmann::json::parse( result->body );

          if ( j.find( "token" ) == j.end() ) {
            spdlog::error( "OCI::Registry::Client::auth Failed: {}", j.dump( 2 ) );
          } else {
            j.get_to( ctr_ );
            spdlog::debug( "OCI::Registry::Client::auth recieved token: {} issued_at: {} expires_in: {}",
                ctr_.token,
                std::chrono::duration_cast<std::chrono::seconds>( ctr_.issued_at.time_since_epoch() ).count(),
                ctr_.expires_in.count() );
          }
        }
        break;
      case HTTP_CODE::Unauthorized:
        spdlog::error( "OCI::Registry::Client::auth Unauthorized {}/{}", domain, location );

        break;
      case HTTP_CODE::Service_Unavail:
        std::this_thread::yield();
        spdlog::warn( "OCI::Registry::Client::auth Service Unavailable retrying Location: {}", location );
        auth( headers, scope );
        break;
      default:
        spdlog::error( "OCI::Registry::Client::auth status: {} Location: {} Body: {}", result->status, location, result->body );
        break;
      }
    }
  }
}

auto OCI::Registry::Client::authHeaders() const -> httplib::Headers {
  httplib::Headers retVal{};

  if ( ctr_.token.empty() ) {
    spdlog::debug( "OCI::Registry::Client::authHeaders TokenResponse token is empty" );
  } else if ( ( ctr_.issued_at + ctr_.expires_in ) <= std::chrono::system_clock::now() ) {
    spdlog::debug( "OCI::Registry::Client::authHeaders TokenResponse token is expired issued_at: {} expires_at: {} current_time: {}",
            std::chrono::duration_cast<std::chrono::seconds>( ctr_.issued_at.time_since_epoch() ).count(),
            ctr_.expires_in.count(),
            std::chrono::duration_cast<std::chrono::seconds>( std::chrono::system_clock::now().time_since_epoch() ).count()
        );
  } else {
    retVal = httplib::Headers{
        { "Authorization", "Bearer " + ctr_.token } // only return this if token is valid
    };
  }

  return retVal;
}

auto OCI::Registry::Client::catalog() -> const OCI::Catalog& {
  static Catalog retVal;

  spdlog::warn( "OCI::Registry::Client::catalog Not implemented" );

  return retVal;
}

auto OCI::Registry::Client::copy() -> std::unique_ptr< OCI::Base::Client > {
  return std::make_unique< OCI::Registry::Client >( *this );
}

auto OCI::Registry::Client::fetchBlob( const std::string &rsrc, SHA256 sha,
                                       std::function< bool( const char *, uint64_t ) > &call_back ) -> bool {
  ToggleLocationGuard< decltype( cli_ ) > tlg{ cli_, false };
  bool retVal = true;
  auto client = cli_;

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

    if ( domain != domain_ ) {
      if ( proto == "https" ) {
        client = std::make_shared< httplib::SSLClient >( domain, SSL_PORT );
      } else {
        client = std::make_shared< httplib::Client >( domain, DOCKER_PORT );
      }

      client->set_logger( &http_logger );
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
  ToggleLocationGuard< decltype( cli_ ) > tlg{ cli_, false };

  auto client = cli_;

  auto location = "/v2/" + im.name + "/blobs/" + sha;
  auto res      = cli_->Head( location.c_str(), authHeaders() );

  if ( HTTP_CODE( res->status ) == HTTP_CODE::Unauthorized ) {
    auth( res->headers,
          "repository:" + im.name + ":pull" ); // auth modifies the headers, so should auth return headers???

    res = client->Head( location.c_str(), authHeaders() );
  }

  if ( res->has_header( "Location" ) ) {
    std::string proto;
    std::string domain;

    std::tie( proto, domain, location ) = splitLocation( res->get_header_value( "Location" ) );

    if ( domain != domain_ ) {
      if ( proto == "https" ) {
        client = std::make_shared< httplib::SSLClient >( domain, SSL_PORT );
      } else {
        client = std::make_shared< httplib::Client >( domain, DOCKER_PORT );
      }

      client->set_logger( &http_logger );
    }

    res = client->Head( location.c_str(), authHeaders() );
  }

  return HTTP_CODE( res->status ) == HTTP_CODE::OK or HTTP_CODE( res->status ) == HTTP_CODE::Found;
}

auto OCI::Registry::Client::hasBlob( const Schema2::ImageManifest &im, const std::string &target, SHA256 sha ) -> bool {
  (void)target;
  ToggleLocationGuard< decltype( cli_ ) > tlg{ cli_, false };

  auto client   = cli_;
  auto location = "/v2/" + im.name + "/blobs/" + sha;
  auto res      = client->Head( location.c_str(), authHeaders() );

  if ( res == nullptr ) {
    throw std::runtime_error( "OCI::Registry::Client::hasBlob recieved NULL requested head on '" + location + "'" );
  }

  if ( HTTP_CODE( res->status ) == HTTP_CODE::Unauthorized ) {
    auth( res->headers, "repository:" + im.name + ":pull" );

    res = client->Head( location.c_str(), authHeaders() );
  }

  if ( res->has_header( "Location" ) ) {
    std::string proto;
    std::string domain;

    std::tie( proto, domain, location ) = splitLocation( res->get_header_value( "Location" ) );

    if ( domain != domain_ ) {
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
  std::shared_ptr< httplib::Response > res{ nullptr };

  std::uint16_t port{0};
  std::string   proto;
  std::string   domain;

  if ( patch_location_.empty() ) {
    spdlog::debug( "OCI::Registry::Client::putBlob starting {}", blob_sha );

    headers.emplace( "Host", domain_ );
    res = cli_->Post( ( "/v2/" + im.name + "/blobs/uploads/" ).c_str(), headers, "", "" );

    if ( res == nullptr ) {
      throw std::runtime_error( "OCI::Registry::Client::putBlob received NULL starting " + blob_sha );
    }

    while ( res != nullptr and patch_location_.empty() ) {
      switch ( HTTP_CODE( res->status ) ) {
      case HTTP_CODE::Accepted:
        spdlog::debug( "OCI::Registry::Client::putBlub Accepted {}:{}", target, blob_sha );

        {
          std::tie( proto, domain, patch_location_ ) = splitLocation( res->get_header_value( "Location" ) );

          if ( patch_cli_ == nullptr ) {
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
              patch_cli_ = std::make_unique< httplib::SSLClient >( domain, port );
            } else {
              patch_cli_ = std::make_unique< httplib::Client >( domain, port );
            }

            patch_cli_->set_logger( &http_logger );
          }
        }

        break;
      case HTTP_CODE::Bad_Request:
        throw std::runtime_error( "OCI::Registry::Client::putBlob HTTP Bad Request " + target + ":" + blob_sha );
        break;
      case HTTP_CODE::Unauthorized:
        spdlog::info( "OCI::Registry::Client First auth" );
        auth( res->headers, "repository:" + im.name + ":pull,push" );

        headers = authHeaders();
        headers.emplace( "Host", domain_ );

        res = cli_->Post( ( "/v2/" + im.name + "/blobs/uploads/" ).c_str(), headers, "", "" );
        break;
      default:
        throw std::runtime_error( "OCI::Registry::Client Upload initiate Status: " + std::to_string( res->status ) );
        break;
      }
    }
  }

  do {
    if ( last_offset_ == 0 ) {
      headers.emplace( "Content-Range", std::to_string( last_offset_ ) + "-" + std::to_string( last_offset_ + blob_part_size - 1 ) );
    } else {
      headers.emplace( "Content-Range", std::to_string( last_offset_ ) + "-" + std::to_string( last_offset_ + blob_part_size ) );
    }

    headers.emplace( "Content-Length", std::to_string( blob_part_size ) );

    if ( last_offset_ + blob_part_size == total_size or blob_part_size == total_size ) {
      spdlog::debug( "OCI::Registry::Client::putBlob Finalizing upload for Blob {}", blob_sha );

      res = patch_cli_->Put( ( patch_location_ + "?digest=" + blob_sha ).c_str(), headers, { blob_part, blob_part_size }, "application/octet-stream" );

      patch_cli_      = nullptr;
      patch_location_ = "";
      last_offset_    = 0;
    } else {
      // This is slow, interesting enough, using Get in the same library is fast -- lets look here
      res = patch_cli_->Patch( patch_location_.c_str(), headers, { blob_part, blob_part_size }, "application/octet-stream" );
    }

    if ( res == nullptr ) {
      throw std::runtime_error( "OCI::Registry::Client::putBlob received NULL starting " + blob_sha );
    }

    switch ( HTTP_CODE( res->status ) ) {
    case HTTP_CODE::Bad_Request:
      throw std::runtime_error( "OCI::Registry::Client::putBlob HTTP Bad Request " + target + ":" + blob_sha );
      break;
    case HTTP_CODE::Unauthorized:
      auth( res->headers, "repository:" + im.name + ":pull,push" );
      headers = authHeaders();

      break;
    case HTTP_CODE::Accepted:
    case HTTP_CODE::No_Content:
      {
        std::tie( proto, domain, patch_location_ ) = splitLocation( res->get_header_value( "Location" ) );
        auto range   = res->get_header_value( "Range" );
        last_offset_ = std::stoul( range.substr( range.find( '-' ) + 1 ) );
      }

      retVal = true;
      break;
    default:
      throw std::runtime_error( "OCI::Registry::Client Patch Status: " + std::to_string( res->status ) );
      break;
    }
  } while ( not retVal );

  return retVal;
}

void OCI::Registry::Client::fetchManifest( Schema1::ImageManifest &im, Schema1::ImageManifest const &request ) {
  im.raw_str     = fetchManifest( im.mediaType, request.name, request.requestedTarget );
  auto json_body = nlohmann::json::parse( im.raw_str );

  json_body.get_to( im );

  if ( im.name.empty() ) {
    im.name = request.name;
  }

  im.originDomain = request.originDomain; // This is just for sync from a Registry to a Directory
}

void OCI::Registry::Client::fetchManifest( Schema1::SignedImageManifest &      sim,
                                           Schema1::SignedImageManifest const &request ) {
  sim.raw_str     = fetchManifest( sim.mediaType, request.name, request.requestedTarget );
  auto json_body = nlohmann::json::parse( sim.raw_str );

  json_body.get_to( sim );

  if ( sim.name.empty() ) {
    sim.name = request.name;
  }

  sim.originDomain = request.originDomain; // This is just for sync from a Registry to a Directory
}

void OCI::Registry::Client::fetchManifest( Schema2::ManifestList &ml, Schema2::ManifestList const &request ) {
  ml.raw_str = fetchManifest( ml.mediaType, request.name, request.requestedTarget );

  if ( not ml.raw_str.empty() ) {
    auto json_body = nlohmann::json::parse( ml.raw_str );

    if ( not json_body.empty() ) {
      json_body.get_to( ml );

      if ( ml.name.empty() ) {
        ml.name = request.name;
      }

      if ( request.originDomain.empty() ) {
        ml.originDomain = domain_;
      } else {
        ml.originDomain = request.originDomain;
      }

      ml.requestedTarget = request.requestedTarget;
    }
  }
}

void OCI::Registry::Client::fetchManifest( Schema2::ImageManifest &im, Schema2::ImageManifest const &request ) {
  im.raw_str     = fetchManifest( im.mediaType, request.name, request.requestedDigest );

  if ( not im.raw_str.empty() ) {
    auto json_body = nlohmann::json::parse( im.raw_str );

    json_body.get_to( im );

    if ( im.name.empty() ) {
      im.name = request.name;
    }

    if ( request.originDomain.empty() ) {
      im.originDomain = domain_;
    } else {
      im.originDomain = request.originDomain; // This is just for sync from a Registry to a Directory
    }

    im.requestedTarget = request.requestedTarget;
    im.requestedDigest = request.requestedDigest;
  }
}

auto OCI::Registry::Client::fetchManifest( const std::string &mediaType, const std::string &resource,
                                           const std::string &target ) -> std::string const {
  auto location = "/v2/" + resource + "/manifests/" + target;
  auto headers  = authHeaders();

  std::string retVal;

  headers.emplace( "Accept", mediaType );

  auto res = cli_->Get( location.c_str(), headers );

  if ( res == nullptr ) {
    throw std::runtime_error( "OCI::Registry::Client::fetchManifest request error, returned NULL " + resource + ":" + target );
  } else {
    switch ( HTTP_CODE( res->status ) ) {
    case HTTP_CODE::Unauthorized:
      spdlog::warn( "OCI::Registry::Client::fetchManifest recieved HTTP_CODE::Unauthorized for '{}'", location );

      if ( auth_retry_ ) {
        auth_retry_ = false;
        auth( res->headers, "repository:" + resource + ":pull" ); // auth modifies the headers, so should auth return headers???

        retVal = fetchManifest( mediaType, resource, target );
      } else {
        throw std::runtime_error( "OCI::Registry::Client::fetchManifest recieved HTTP_CODE::Unauthorized for " + location );
      }

      break;
    case HTTP_CODE::OK:
      auth_retry_ = true;
      retVal = res->body;
      break;
    case HTTP_CODE::Not_Found:
      auth_retry_ = true;
      spdlog::warn( "OCI::Registry::Client::fetchManifest request Manifest Not_Found {} {}:{}", mediaType, resource,
                    target );
      break;
    default:
      auth_retry_ = true;
      spdlog::error( "OCI::Registry::Client::fetchManifest {}", location );
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

  auto res =
      cli_->Put( ( "/v2/" + ml.name + "/manifests/" + target ).c_str(), authHeaders(), ml.raw_str, ml.mediaType.c_str() );

  if ( res == nullptr ) {
  } else {
    if ( HTTP_CODE( res->status ) == HTTP_CODE::Unauthorized ) {
      auth( res->headers, "repository:" + ml.name + ":push" );

      res = cli_->Put( ( "/v2/" + ml.name + "/manifests/" + target ).c_str(), authHeaders(), ml.raw_str,
                       ml.mediaType.c_str() );
    }

    switch ( HTTP_CODE( res->status ) ) {
    case HTTP_CODE::Created:
      retVal = true;
      break;
    default:
      spdlog::error( "OCI::Registry::Client::putManifest Schema2::ImageManifest {}", ml.name );
    }
  }

  return retVal;
}

auto OCI::Registry::Client::putManifest( Schema2::ImageManifest const &im, std::string const &target ) -> bool {
  bool           retVal = false;

  auto res =
      cli_->Put( ( "/v2/" + im.name + "/manifests/" + target ).c_str(), authHeaders(), im.raw_str, im.mediaType.c_str() );

  if ( res == nullptr ) {
  } else {
    if ( HTTP_CODE( res->status ) == HTTP_CODE::Unauthorized ) {
      auth( res->headers, "repository:" + im.name + ":push" );

      res = cli_->Put( ( "/v2/" + im.name + "/manifests/" + target ).c_str(), authHeaders(), im.raw_str,
                       im.mediaType.c_str() );
    }

    switch ( HTTP_CODE( res->status ) ) {
    case HTTP_CODE::Created:
      retVal = true;
      break;
    default:
      spdlog::error( "OCI::Registry::Client::putManifest Schema2::ImageManifest {}:{}", im.name, target );
      spdlog::error( im.raw_str );
    }
  }

  return retVal;
}

auto OCI::Registry::Client::tagList( const std::string &rsrc ) -> OCI::Tags {
  Tags retVal;

  auto location = "/v2/" + rsrc + "/tags/list";
  auto res      = cli_->Get( location.c_str(), authHeaders() );

  if ( HTTP_CODE( res->status ) == HTTP_CODE::Unauthorized ) {
    auth( res->headers, "repository:" + rsrc + ":pull" );

    res = cli_->Get( location.c_str(), authHeaders() );
  }

  if ( HTTP_CODE( res->status ) == HTTP_CODE::OK ) {
    retVal = nlohmann::json::parse( res->body ).get< Tags >();
  } else {
    spdlog::error( "OCI::Registry::Client::tagList {}", location );
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

  auto res = cli_->Get( "/v2/" );

  if ( res == nullptr ) { // Most likely an invalid domain or port
    retVal = false;
    //throw std::runtime_error( "OCI::Registry::Client::ping recieved NULL on response" );
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
  auto       res      = cli_->Get( location.c_str(), authHeaders() );

  if ( HTTP_CODE( res->status ) == HTTP_CODE::Unauthorized ) {
    auth( res->headers, "repository:" + rsrc + ":pull" );
    res = cli_->Get( location.c_str(), authHeaders() );
  }

  return HTTP_CODE( res->status ) == HTTP_CODE::OK;
}

auto OCI::Registry::Client::swap( Client &other ) -> void {
  std::swap( domain_, other.domain_ );
  std::swap( username_, other.username_ );
  std::swap( password_, other.password_ );
  std::swap( cli_, other.cli_ );
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
    ss >> std::get_time( &tm, "%Y-%m-%dT%H:%M:%S" ); // EXAMPLE 2020-04-20T11:52:16.177118311Z
    
    std::lock_guard< std::mutex > lg( TIME_ZONE_MUTEX );
    ctr.issued_at = std::chrono::system_clock::from_time_t( std::mktime( &tm ) );
  } else {
    ctr.issued_at = std::chrono::system_clock::now();
  }

  if ( j.find( "refresh_token" ) != j.end() ) {
    j.at( "refresh_token" ).get_to( ctr.refresh_token );
  }
}
