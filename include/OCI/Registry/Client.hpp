#pragma once
#include <OCI/Schema1.hpp>
#include <OCI/Schema2.hpp>
#include <OCI/Tags.hpp>
#include <OCI/Registry/Error.hpp>
#include <string>
#include <vector>
#include <iostream> // Will want to remove this at somepoint
#include <nlohmann/json.hpp> // https://github.com/nlohmann/json

#define CPPHTTPLIB_OPENSSL_SUPPORT = 1
#include <httplib.h> // https://github.com/yhirose/cpp-httplib

namespace OCI { // https://docs.docker.com/registry/spec/api/
  namespace Registry {
    class Client {
    public:
      Client( std::string const & domain );
      Client( std::string const & domain, std::string const & username, std::string const & password );
      ~Client();

      void auth( std::string rsrc );

      void inspect( std::string rsrc );

      template< class Schema_t >
      Schema_t manifest( std::string rsrc, std::string target );

      void pull( Schema1::ImageManifest im );
      void pull( Schema2::ManifestList ml ); // multi-arch/platform pull
      void pull( Schema2::ImageManifest im ); // single schema v2 pull

      void push( Schema1::ImageManifest im );
      void push( Schema2::ManifestList ml );

      Tags tagList( std::string rsrc );

      bool pingResource( std::string rsrc );
//      bool ping();
    protected:
      httplib::Headers defaultHeaders(); 
      httplib::SSLClient  _cli;
    private:
      std::string   _token;
      std::string   _username;
      std::string   _password;
    };
  } // namespace Registry
} // namespace oci


// IMPLEMENTATION
// OCI::Registry::Client
OCI::Registry::Client::Client( std::string const & domain ) : _cli( domain, 443 ) {
  _cli.set_follow_location( true );
}

OCI::Registry::Client::~Client() = default;

void OCI::Registry::Client::auth( std::string rsrc ) {
  if ( not _username.empty() and not _password.empty() )
    _cli.set_basic_auth( _username.c_str(), _password.c_str() );

  auto res = _cli.Get( std::string( "/v2/" + rsrc ).c_str(), defaultHeaders() );

  if ( res == nullptr ) { // need to handle timeouts a little more gracefully
    std::abort(); // Need to figure out how this API is going to handle errors
    // And I keep adding to the technical debt
  } else if ( res->status == 404 ) {
    std::cerr << "/v2/" << rsrc << std::endl;
    std::cerr << rsrc << " " << res->body << std::endl;
    std::abort();
  }

  if ( res->status != 200 ) { // This hot mess just to get the auth endpoint
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
      httplib::SSLClient client( realm, 443 );
      auto result = client.Get( ( endpoint + "?service=" + service + "&scope=" + scope ).c_str() );

      if ( result == nullptr ) {
        std::abort(); // Need to figure out how this API is going to handle errors
        // And I keep adding to the technical debt
      }

      //std::cout << nlohmann::json::parse( result->body ).dump( 2 ) << std::endl;

      _token = nlohmann::json::parse( result->body ).at( "token" );
    }
  }
}

httplib::Headers OCI::Registry::Client::defaultHeaders() {
  return {
    { "Authorization", "Bearer " + _token }
  };
}

void OCI::Registry::Client::inspect( std::string base_rsrc ) {
  using namespace std::string_literals;

  auto target = "latest"s; // Target can be tag or sha256 string
  auto rsrc   = base_rsrc;

  if ( base_rsrc.find( ':' ) != std::string::npos ) {
    target = base_rsrc.substr( base_rsrc.find( ':' ) + 1 );
    rsrc   = base_rsrc.substr( 0, base_rsrc.find( ':' ) );
  }

  auto tags         = tagList( rsrc );

  auto manifestList = manifest< Schema2::ManifestList >( rsrc, target );

  if ( manifestList.schemaVersion == 1 ) { // Fall back to Schema1
      auto image_manifest = manifest< Schema1::ImageManifest >( rsrc, target );
  } else if ( manifestList.schemaVersion == 2 ) {
    // need to do more than this, doing this instead of to_json for the moment
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
      auto image_manifest = manifest< Schema2::ImageManifest >( rsrc, target );
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

template< class Schema_t >
Schema_t OCI::Registry::Client::manifest( std::string rsrc, std::string target ) {
  Schema_t retVal;
  auto headers = defaultHeaders();
  headers.emplace( "Accept", retVal.mediaType );

  auto res = _cli.Get( std::string( "/v2/" + rsrc + "/manifests/" + target ).c_str(), headers );

  if ( res == nullptr ) {
    std::abort();
  }

  if ( res->status != 200 ) { // TODO: Should find the reason instead of assuming
    auth( rsrc + "/manifests/" + target ); // auth modifies the headers, so should auth return headers???
    headers = defaultHeaders();
    headers.emplace( "Accept", retVal.mediaType );

    res = _cli.Get( std::string( "/v2/" + rsrc + "/manifests/" + target ).c_str(), headers );

    if ( res == nullptr ) {
      std::abort();
    }
  }

  if ( res->status == 200 ) {
    retVal = nlohmann::json::parse( res->body ).get< Schema_t >();

    if ( retVal.name.empty() )
      retVal.name = rsrc;
  } else {
    std::cerr << res->body << std::endl;
  }

  return retVal;
}

OCI::Tags OCI::Registry::Client::tagList( std::string rsrc ) {
  Tags retVal;

  auto res = _cli.Get( std::string( "/v2/" + rsrc + "/tags/list" ).c_str(), defaultHeaders() );

  if ( res->status != 200 ) {
    auth( rsrc + "/tags/list" );

    res = _cli.Get( std::string( "/v2/" + rsrc + "/tags/list" ).c_str(), defaultHeaders() );

    if ( res == nullptr ) {
      std::abort();
    }

    if ( res->status == 200 ) {
      retVal = nlohmann::json::parse( res->body ).get< Tags >();
    } else {
      std::cerr << res->body << std::endl;
    }
  }

  return retVal;
}

void OCI::Registry::Client::pull( Schema2::ManifestList ml ) {
  using namespace std::string_literals;
  for ( auto const& im: ml.manifests ) {
    auto image_manifest = manifest< Schema2::ImageManifest >( ml.name, im.digest );

    for ( auto const& iml: image_manifest.layers ) {
      _cli.Get( ( "/v2/"s + ml.name + "/blobs/"s + iml.digest ).c_str(), [](long long len, long long total) {
          printf("%lld / %lld bytes => %d%% complete\r",
            len, total,
            (int)(len*100/total));
          return true; // return 'false' if you want to cancel the request.
        } );
      std::cout << std::endl;
    }
  }
}

bool OCI::Registry::Client::pingResource( std::string rsrc ) {
  bool retVal = false;

  if ( _token.empty() ) {
    auto res  = _cli.Get( std::string( "/v2/" + rsrc ).c_str(), defaultHeaders() );
    retVal    = res->status == 200;
  }

  return retVal;
}
//
//bool OCI::Registry::Client::ping() {
//  auto res = _cli.Get( "/v2/", defaultHeaders() );
//  return res->status == 200;
//}
