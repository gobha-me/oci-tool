#include <iostream>
#include <OCI/Registry/Client.hpp>
#include <OCI/Extensions/Dir.hpp>
#include <OCI/Copy.hpp>

// This spawned from the need of doing a multi arch sync, which requires a multi arch copy
//  and learn how REST interfaces can be implemented, so going the 'hard' route was a personal choice

// The initial pieces is an OCI compatible client, but ultimately will need a "higher" level abstraction
//   so that other pieces can be swapped with a "similar/like" interface

// The layout is expected to change, and I am hoping for constructive feedback

// TODO: replace std::cout, std::cerr and std::about() with proper error handling and reporting

int main( int argc, char ** argv ) {
  using namespace std::string_literals;
  // simply we start with requiring at least two options, the command and the uri
  if ( argc < 2 ) return EXIT_FAILURE;

  // TODO: ARG Parser -- already adding technical debt
  //      --username / -u
  //      --password / -p
  //      for a copy from one to another this maybe a per command arg -- hmm
  //        also if the copy is to a dir, then kinda a mute point
  //      this brings to mind that there will be general "client" args, but the commands
  //        themselves will have arguments
  //      I have found a couple of header libraries that look promising
  //        https://github.com/p-ranav/argparse
  //        https://github.com/sailormoon/flags
  //        they both have something to offer, but don't really offer the modern command
  //          style structure of cli interfaces
  //          $ exec <--flags 0-n> command <--flags 0-n> [positional args or (subcommands <--flags>) 0-n] ...
  auto command    = std::string( argv[1] );
  auto uri        = std::string( argv[2] );
  auto proto_itr  = uri.find( ":" );
  std::string proto, location;

  if ( proto_itr != std::string::npos ) {
    proto     = uri.substr( 0, proto_itr );
    location  = uri.substr( proto_itr + 1 );
  }

  // If URI is of target docker

  // command is the basic operation we want to perform against the registry for a given URI

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
    //OCI::Registry::Client client2( other_domain, username, password );
    //OCI::Extensions::Dir dir( "<path>" );

    // So according to the API doc this is how to handle the API, all buried in the implementation
    // 1. attempt operation
    // 2. review response -- the container auth instructions are in header
    // 3. send auth request
    // 4. parse auth responce for token
    // 5. retry operation
    // 6. review response
    //client1.inspect( rsrc ); // there is no arg parsing ATM to determine what to do

    OCI::Copy( rsrc, target, client1, client1 );
  }

  return EXIT_SUCCESS;
}
