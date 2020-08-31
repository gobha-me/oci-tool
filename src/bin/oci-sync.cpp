#include <OCI/Factory.hpp>
#include <OCI/Extensions/Yaml.hpp>
#include <OCI/Sync.hpp>
#include <Yaml.hpp>
#include <iostream>
#include <args.hxx>
#include <spdlog/spdlog.h>

// So far this will only work for the following combinations
// yaml -> dir
// yaml -> docker (unauthenticated)
// dir  -> dir
// dir  -> docker (unauthenticated)
auto main( int argc, char ** argv ) -> int {
  using namespace std::string_literals;

  args::ArgumentParser            parser( "Multi architecture OCI sync tool" );
  args::HelpFlag                  help( parser, "help", "Display this help message", { 'h', "help" } );
  args::CounterFlag               verbose( parser, "VERBOSE", "increase logging", { 'v', "verbose" } );
  args::ValueFlag< std::string >  dest_username( parser, "USERNAME", "User to authenticate to the destination registry", { 'u', "username" } );
  args::ValueFlag< std::string >  dest_password( parser, "PASSWORD", "PASSWORD to authenticate to the destination registry", { 'p', "password" } );
  args::Group                     group( parser, "", args::Group::Validators::All ); // NOLINT API Defined outside this application Core Guidelines Slicing
  args::Positional< std::string > src_arg( group, "<proto>:<uri>", "Images source" );
  args::Positional< std::string > dest_arg( group, "<proto>:<uri>", "Images destination" );
  args::CompletionFlag            completion( parser, { "complete" } );

  try {
    parser.ParseCLI( argc, argv );
  } catch ( args::Help& ) {
    std::cout << parser;

    return EXIT_FAILURE;
  } catch ( args::ParseError& e ) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;

    return EXIT_FAILURE;
  } catch ( args::ValidationError& e ) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;

    return EXIT_FAILURE;
  }

  switch ( verbose.Get() ) {
    case 1:
      spdlog::set_level( spdlog::level::debug );
      break;
    case 2:
      spdlog::set_level( spdlog::level::trace );
      break;
    default:
      spdlog::set_level( spdlog::level::info );
      break;
  }

  spdlog::set_pattern("[%H:%M:%S] [%^%l%$] [thread %t] %v");

  std::shared_ptr< OCI::Base::Client > source;

  std::string src_username;
  std::string src_password;
  
  auto src_proto_itr  = src_arg.Get().find( ':' );
  auto dest_proto_itr = dest_arg.Get().find( ':' );

  if ( src_proto_itr == std::string::npos ) {
    std::cout << parser;

    return EXIT_FAILURE;
  }

  if ( dest_proto_itr == std::string::npos or dest_arg.Get().find( "yaml" ) != std::string::npos ) {
    // yaml as a proto is not a valid destination
    // valid distinations are
    //  - base dirs
    //  - docker registry domains
    // the repo name and tag info comes from the source
    //  this is by design as we are "cloning" the source
    //  this is not made for renaming or tagging, might be a
    //  feature to consider for oci-copy
    std::cout << parser;

    return EXIT_FAILURE;
  }

  // need to ensure src and dest proto are within a supported set
  //  as of today that is: yaml docker dir

  auto src_proto     = src_arg.Get().substr( 0, src_proto_itr );
  auto src_location  = src_arg.Get().substr( src_proto_itr + 1 );

  auto dest_proto    = dest_arg.Get().substr( 0, dest_proto_itr );
  auto dest_location = dest_arg.Get().substr( dest_proto_itr + 1 );
  
  auto destination = OCI::CLIENT_MAP.at( dest_proto )( dest_location, dest_username.Get(), dest_password.Get() );

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
