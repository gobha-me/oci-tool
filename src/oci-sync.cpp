#include <iostream>
#include <OCI/Registry/Client.hpp>
#include <OCI/Extensions/Dir.hpp>
#include <OCI/Copy.hpp>

// This spawned from the need of doing a multi arch sync, which requires a multi arch copy
//  and learn how REST interfaces can be implemented, so going the 'hard' route was a personal choice

int main( int argc, char ** argv ) {
  using namespace std::string_literals;
  // simply we start with requiring at least two options, the command and the uri
  if ( argc < 2 ) return EXIT_FAILURE;

  auto command    = std::string( argv[1] );
  auto uri        = std::string( argv[2] );
  auto proto_itr  = uri.find( ":" );
  std::string proto, location;

  if ( proto_itr != std::string::npos ) {
    proto     = uri.substr( 0, proto_itr );
    location  = uri.substr( proto_itr + 1 );
  }

  if ( proto == "docker" ) { // TODO: need search domains, on input without a domain domain == rsrc
    location = location.substr( 2 ); // the // is not needed for this http client library
    auto domain = location.substr( 0, location.find( '/' ) );
    auto rsrc   = location.substr( location.find( '/') + 1 );
    auto target = "latest"s;

    // if domain not provided use /etc/containers/registries.conf - INI? This process turns into a loop?
    if ( domain == rsrc ) { // domain was not provided
    }

    // in uri docker will translate to https
    // if docker.io use registry-1.docker.io as the site doesn't redirect
    if ( domain == "docker.io" ) {
      // either docker.io should work, or they should have a proper redirect implemented
      domain = "registry-1.docker.io";
    }

    if ( rsrc.find( '/' ) == std::string::npos )
      rsrc = "library/" + rsrc; // set to default namespace if non provided

    if ( rsrc.find( ':' ) != std::string::npos ) {
      rsrc    = rsrc.substr( 0, rsrc.find( ':' ) );
      target  = rsrc.substr( rsrc.find( ':' ) + 1 );
    }

    // std::cout << command << " " << proto << " " << domain << " " << rsrc << std::endl;

    OCI::Registry::Client client1( domain );
    //OCI::Extensions::Dir dir( "<path>" );

    OCI::Sync( rsrc, client1, client1 );
  }

  return EXIT_SUCCESS;
}
