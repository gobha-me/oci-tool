#include <OCI/Factory.hpp>
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
  
  auto destination = CLIENT_MAP.at( dest_proto )( dest_location, dest_username, dest_password );

  if ( src_proto == "yaml" ) {
    Yaml::Node root_node; // need a new Yaml parser, this one doesn't follow C++ Iterator standards, which breaks range loops and the STL algorithms
    Yaml::Parse( root_node, src_location.c_str() ); // c-string for filename, std::string for a string to parse, WTF, should be two different functions

    for ( auto source_node = root_node.Begin(); source_node != root_node.End(); source_node++ ) {
      auto domain       = (*source_node).first;
      auto images_node  = (*source_node).second[ "images" ];
      auto username     = (*source_node).second[ "username" ].As< std::string >();
      auto password     = (*source_node).second[ "password" ].As< std::string >();

      auto source = CLIENT_MAP.at( "docker" )( domain, username, password );;

      for ( auto image_node = images_node.Begin(); image_node != images_node.End(); image_node++ ) {
        auto resource = (*image_node).first;

        if ( resource.find( '/' ) == std::string::npos ) {
          resource.insert( 0, "library/" ); // set to default namespace if non provided
        }

        if ( (*image_node).second.IsSequence() ) {
          std::vector< std::string > tags;

          for ( auto tag_node = (*image_node).second.Begin(); tag_node != (*image_node).second.End(); tag_node++ ) {
            tags.push_back( (*tag_node).second.As< std::string >() );
          }

          OCI::Sync( resource, tags, source.get(), destination.get() );
        } else {
          OCI::Sync( resource, source.get(), destination.get() );
        }
      }
    }
  } else {
    auto source = CLIENT_MAP.at( src_proto )( src_location, src_password, src_password );

    // need a 'resource', but without will use source which assumes _catalog is implemented or available
    OCI::Sync( source.get(), destination.get() );
  }

  return EXIT_SUCCESS;
}
