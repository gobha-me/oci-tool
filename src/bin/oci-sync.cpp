#include <OCI/Extensions/Yaml.hpp>
#include <OCI/Factory.hpp>
#include <OCI/Sync.hpp>
#include <args.hxx> // https://github.com/Taywee/args
#include <csignal>
#include <indicators.hpp>
#include <iostream>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h> // https://github.com/gabime/spdlog

class indicators_cursor_guard {
public:
  indicators_cursor_guard() { indicators::show_console_cursor( false ); }

  indicators_cursor_guard( indicators_cursor_guard const & ) = delete;
  indicators_cursor_guard( indicators_cursor_guard && )      = delete;

  auto operator=( indicators_cursor_guard const & ) = delete;
  auto operator=( indicators_cursor_guard && ) = delete;

  ~indicators_cursor_guard() { indicators::show_console_cursor( true ); }
};

// So far this will only work for the following combinations
// yaml -> dir
// yaml -> docker (unauthenticated)
// dir  -> dir
// dir  -> docker (unauthenticated)
auto main( int argc, char **argv ) -> int {
  using namespace std::string_literals;
  signal( SIGINT, []( int signum ) {
    indicators::show_console_cursor(
        true ); // FIXME THIS IS NOT WORKING, THREAD SAFE SIGNALS, OR MODERN SIGNALS NEED TO BE RESEARCHED
    std::exit( signum );
  } );

  indicators_cursor_guard icg{};

  args::ArgumentParser           parser( "Multi architecture OCI sync tool" );
  args::HelpFlag                 help( parser, "help", "Display this help message", { 'h', "help" } );
  args::CounterFlag              verbose( parser, "VERBOSE", "increase logging", { 'v', "verbose" } );
  args::ValueFlag< std::string > log_file( parser, "", "", { 'l', "log-file" } );
  args::ValueFlag< std::string > dest_username( parser, "USERNAME", "User to authenticate to the destination registry",
                                                { 'u', "username" } );
  args::ValueFlag< std::string > dest_password(
      parser, "PASSWORD", "PASSWORD to authenticate to the destination registry", { 'p', "password" } );
  args::Group group( parser, "", args::Group::Validators::All ); // NOLINT(cppcoreguidelines-slicing)
  args::Positional< std::string > src_arg( group, "<proto>:<uri>", "Images source" );
  args::Positional< std::string > dest_arg( group, "<proto>:<uri>", "Images destination" );
  args::CompletionFlag            completion( parser, { "complete" } );

  try {
    parser.ParseCLI( argc, argv );
  } catch ( args::Help & ) {
    std::cout << parser;

    return EXIT_FAILURE;
  } catch ( args::ParseError &e ) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;

    return EXIT_FAILURE;
  } catch ( args::Completion &e ) {
    std::cerr << e.what();

    return EXIT_SUCCESS;
  } catch ( args::ValidationError &e ) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;

    return EXIT_FAILURE;
  }

  if ( not log_file.Get().empty() ) {
    spdlog::set_default_logger( spdlog::basic_logger_mt( "basic_logger", log_file.Get() ) );
  }

  switch ( verbose.Get() ) {
  case 1:
    spdlog::set_level( spdlog::level::warn );
    break;
  case 2:
    spdlog::set_level( spdlog::level::info );
    break;
  case 3:
    spdlog::set_level( spdlog::level::debug );
    break;
  case 4:
    spdlog::set_level( spdlog::level::trace );
    break;
  default:
    if ( log_file.Get().empty() ) {
      spdlog::set_level( spdlog::level::off );
    } else {
      spdlog::set_level( spdlog::level::err );
    }

    break;
  }

  spdlog::flush_every( std::chrono::seconds( 1 ) );
  spdlog::set_pattern( "[%H:%M:%S] [%^%l%$] [thread %t] %v" );

  std::shared_ptr< OCI::Base::Client > source;

  std::string src_username;
  std::string src_password;

  auto src_proto_itr  = src_arg.Get().find( ':' );
  auto dest_proto_itr = dest_arg.Get().find( ':' );

  if ( src_proto_itr == std::string::npos ) {
    std::cerr << "Images Source is not properly formated!" << std::endl;
    std::cerr << parser;

    return EXIT_FAILURE;
  }

  if ( dest_proto_itr == std::string::npos or dest_arg.Get().find( "yaml" ) != std::string::npos ) {
    // yaml as a proto is not a valid destination
    // valid distinations are
    //  - base dirs
    //  - docker registry domains
    // the repo name and tag info comes from the source
    //  this is by design as we are "cloning" the source
    //  this is not made for renaming or tagging
    if ( dest_arg.Get().find( "yaml" ) != std::string::npos ) {
      std::cerr << "yaml is not a valid destination" << std::endl;
    } else {
      std::cerr << "Images Destination is not properly formated!" << std::endl;
    }

    std::cerr << parser;

    return EXIT_FAILURE;
  }

  // need to ensure src and dest proto are within a supported set
  //  as of today that is: yaml docker dir

  auto src_proto    = src_arg.Get().substr( 0, src_proto_itr );
  auto src_location = src_arg.Get().substr( src_proto_itr + 1 );

  auto dest_proto    = dest_arg.Get().substr( 0, dest_proto_itr );
  auto dest_location = dest_arg.Get().substr( dest_proto_itr + 1 );

  std::unique_ptr< OCI::Base::Client > destination;
  try {
    destination = OCI::CLIENT_MAP.at( dest_proto )( dest_location, dest_username.Get(), dest_password.Get() );
  } catch ( std::runtime_error const &e ) {
    std::cerr << e.what() << '\n';
    return EXIT_FAILURE;
  }

  // a 'resource', but without will use source which assumes _catalog is implemented or available

  try {
    OCI::Sync sync{};
    if ( src_proto == "yaml" ) {
      auto source = OCI::Extensions::Yaml( src_location );
      sync.execute( &source, destination.get() );
    } else {
      auto source = OCI::CLIENT_MAP.at( src_proto )( src_location, src_password, src_password );
      sync.execute( source.get(), destination.get() );
    }
  } catch ( std::exception const &e ) { // Catch all, Doing this allows stack unwinding on any exception
    std::cerr << "Unhandled exception occured!! " << e.what() << std::endl;

    return EXIT_FAILURE;
  } catch ( ... ) { // Catch anything that doesn't inherit from std::exception
    std::cerr << "Unhandled exception occured!! " << std::endl;

    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
