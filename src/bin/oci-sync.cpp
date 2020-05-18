#include <OCI/Factory.hpp>
#include <OCI/Extensions/Yaml.hpp>
#include <OCI/Sync.hpp>
#include <Yaml.hpp>
#include <iostream>

// This spawned from the need of doing a multi arch sync, which requires a multi arch copy
//  and learn how REST interfaces can be implemented, so going the 'hard' route was a personal choice
// SRC(s), good for now, but would like to extend to a "DirTree" to a Registry, so this would also require a proto

// So far this will only work for the following combinations
// yaml -> dir
// yaml -> docker (unauthenticated)
// dir  -> dir
// dir  -> docker (unauthenticated)
auto main( int argc, char ** argv ) -> int {
  using namespace std::string_literals;

  // simply we start with requiring at least two options, the command and the uri
  if ( argc < 2 ) {
    return EXIT_FAILURE;
  }

  std::shared_ptr< OCI::Base::Client > source;
  std::string src( argv[1] ); // NOLINT
  std::string dest( argv[2] ); // NOLINT

  std::string src_username;
  std::string src_password;

  std::string dest_username;
  std::string dest_password;
  
  auto src_proto_itr  = src.find( ':' );
  auto dest_proto_itr = dest.find( ':' );

  if ( src_proto_itr == std::string:: npos ) {
    std::cerr << "improperly formated source string" << std::endl;
    std::cerr << argv[0] << " <proto>:<uri>" << " <proto>:<uri>" << std::endl; // NOLINT

    return EXIT_FAILURE;
  }

  if ( dest_proto_itr == std::string::npos or dest.find( "yaml" ) != std::string::npos ) {
    // yaml as a proto is not a valid destination
    // valid distinations are
    //  - base dirs
    //  - docker registry domains
    // the repo name and tag info comes from the source
    //  this is by design as we are "cloning" the source
    //  this is not made for renaming or tagging, might be a
    //  feature to consider for oci-copy
    std::cerr << "improperly formated destination string" << std::endl;
    std::cerr << argv[0] << " <proto>:<uri>" << " <proto>:<uri>" << std::endl; // NOLINT

    return EXIT_FAILURE;
  }

  // need to ensure src and dest proto are within a supported set
  //  as of today that is: yaml docker dir

  auto src_proto     = src.substr( 0, src_proto_itr );
  auto src_location  = src.substr( src_proto_itr + 1 );

  auto dest_proto    = dest.substr( 0, dest_proto_itr );
  auto dest_location = dest.substr( dest_proto_itr + 1 );
  
  auto destination = OCI::CLIENT_MAP.at( dest_proto )( dest_location, dest_username, dest_password );

  // a 'resource', but without will use source which assumes _catalog is implemented or available

  if ( src_proto == "yaml" ) {
    auto source = OCI::Extensions::Yaml( src_location );
    OCI::Sync( &source, destination.get() );
  } else {
    auto source = OCI::CLIENT_MAP.at( src_proto )( src_location, src_password, src_password );
    OCI::Sync( source.get(), destination.get() );
  }

  return EXIT_SUCCESS;
}
