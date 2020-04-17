#include <OCI/Extensions/Dir.hpp>
#include <OCI/Registry/Client.hpp>
#include <OCI/Sync.hpp>
#include <Yaml.hpp>
#include <iostream>

// This spawned from the need of doing a multi arch sync, which requires a multi arch copy
//  and learn how REST interfaces can be implemented, so going the 'hard' route was a personal choice
// SRC(s), good for now, but would like to extend to a "DirTree" to a Registry, so this would also require a proto

auto main( int argc, char ** argv ) -> int {
  using namespace std::string_literals;

  // simply we start with requiring at least two options, the command and the uri
  if ( argc < 2 ) {
    return EXIT_FAILURE;
  }

  std::shared_ptr< OCI::Base::Client > source;
  std::shared_ptr< OCI::Base::Client > destination;
  std::string yaml( argv[1] ); // NOLINT
  std::string uri( argv[2] ); // NOLINT
  std::string proto; // proto is equal to docker or dir
  std::string location;

  auto proto_itr  = uri.find( ':' );

  if ( proto_itr == std::string::npos ) {
    std::cerr << "improperly formated destination string" << std::endl;
    std::cerr << "<proto>:<uri>" << std::endl;

    return EXIT_FAILURE;
  }

  proto     = uri.substr( 0, proto_itr );
  location  = uri.substr( proto_itr + 1 );
  

  if ( proto == "dir" ) {
    destination = std::make_shared< OCI::Extensions::Dir >( location );
  } else if ( proto == "docker" ) {
    location = location.substr( 2 ); // the // is not needed for this http client library
    auto domain = location.substr( 0, location.find( '/' ) );

    // in uri docker will translate to https
    // if docker.io use registry-1.docker.io as the site doesn't redirect
    if ( domain == "docker.io" ) {
      // either docker.io should work, or they should have a proper redirect implemented
      domain = "registry-1.docker.io";
    }

    destination = std::make_shared< OCI::Registry::Client >( domain  );
  }

  Yaml::Node root_node; // need a new Yaml parser, this one doesn't follow C++ Iterator standards, which breaks range loops and the STL algorithms
  Yaml::Parse( root_node, yaml.c_str() ); // c-string for filename, std::string for a string to parse, WTF

  for ( auto source_node = root_node.Begin(); source_node != root_node.End(); source_node++ ) {
    auto domain       = (*source_node).first;
    auto images_node  = (*source_node).second[ "images" ];
    auto username     = (*source_node).second[ "username" ].As< std::string >();
    auto password     = (*source_node).second[ "password" ].As< std::string >();

    if ( domain == "docker.io" ) {
      domain = "registry-1.docker.io";
    }

    source = std::make_shared< OCI::Registry::Client >( domain, username, password );

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

  return EXIT_SUCCESS;
}
