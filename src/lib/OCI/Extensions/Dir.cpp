#include <OCI/Extensions/Dir.hpp>
#include <botan/hash.h>
#include <botan/hex.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <mutex>
#include <random>
#include <set>

std::mutex DIR_MUTEX;
std::mutex DIR_MAP_MUT;
std::mutex SEED_MUTEX;

auto genUUID() -> std::string {
  constexpr std::array< char, 62 > const CHARS( {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
    'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
    'u', 'v', 'w', 'x', 'y', 'z',
  } );
  constexpr auto WORD_SIZE = 24;

  std::lock_guard< std::mutex > lg( SEED_MUTEX );
  auto seed = std::chrono::system_clock::now().time_since_epoch().count();

  std::string retVal;
  std::mt19937 generator( seed );
  for ( std::uint16_t index = 0; index != WORD_SIZE; index ++ ) {
   retVal += CHARS[ generator() % ( CHARS.size() - 1 ) ]; // NOLINT
  }

  return retVal;
}

auto validateFile( std::string const& sha, std::filesystem::path const& file ) -> bool {
  bool retVal             = false;
  constexpr auto BUFFSIZE = 4096;

  auto sha256( Botan::HashFunction::create( "SHA-256" ) );

  std::ifstream blob( file, std::ios::binary );
  std::vector< uint8_t > buf( BUFFSIZE );

  while ( blob.good() ) {
    blob.read( reinterpret_cast< char * >( buf.data() ), buf.size() ); // NOLINT
    size_t readcount = blob.gcount();
    sha256->update( buf.data(), readcount );
  }

  std::string sha256_str = Botan::hex_encode( sha256->final(), false );

  if ( sha == "sha256:" + sha256_str ) {
    retVal = true;
  }

  return retVal;
}

OCI::Extensions::Dir::Dir() : _bytes_written( 0 ) {}
OCI::Extensions::Dir::Dir( std::string const& directory ) : _bytes_written( 0 ) {
  auto trailing_slash = directory.find_last_of( '/' );

  if ( trailing_slash == directory.size() - 1 ) {
    _directory = std::filesystem::directory_entry( directory.substr( 0, trailing_slash ) );
  } else {
    _directory = std::filesystem::directory_entry( directory );
  }

  if ( not ( _directory.exists() or _directory.is_directory() ) ) {
    std::cerr << _directory.path() << " does not exist or is not a directory." << std::endl;
    std::abort();
  }
}

OCI::Extensions::Dir::Dir( OCI::Extensions::Dir const& other ) {
  _directory = other._directory;
}

OCI::Extensions::Dir::Dir( OCI::Extensions::Dir && other ) noexcept {
  _directory = std::move( other._directory );
}

auto OCI::Extensions::Dir::operator=( Dir const& other ) -> Dir& { // NOLINT - HANDLE SELF COPIES https://clang.llvm.org/extra/clang-tidy/checks/bugprone-unhandled-self-assignment.html
  _directory = other._directory;

  return *this;
}

auto OCI::Extensions::Dir::operator=( Dir&& other ) noexcept -> Dir& {
  _directory = std::move( other._directory );

  return *this;
}

auto OCI::Extensions::Dir::copy() -> std::unique_ptr< OCI::Base::Client > {
  return std::make_unique< OCI::Extensions::Dir >( *this );
}

auto OCI::Extensions::Dir::catalog() -> OCI::Catalog {
  Catalog retVal;
  auto dir_map = dirMap();

  retVal.repositories.resize( dir_map.size() );

  std::transform( dir_map.begin(), dir_map.end(), back_inserter( retVal.repositories ), []( auto const& pair ) {
    return pair.first;
  } );

  return retVal;
}

auto OCI::Extensions::Dir::fetchBlob( const std::string& rsrc, SHA256 sha, std::function< bool(const char *, uint64_t ) >& call_back ) -> bool {
  bool retVal = true;
  auto dir_map = dirMap();

  std::cout << "Fetching Blob Resource: " << sha << std::endl;
  if ( dir_map.find( rsrc ) != dir_map.end() and dir_map.at( rsrc ).path.find( sha ) != dir_map.at( rsrc ).path.end() ) {
    auto image_path = dir_map.at( rsrc ).path.at( sha ).path();
    constexpr auto BUFFSIZE = 4096;

    std::ifstream blob( image_path, std::ios::binary );
    std::vector< uint8_t > buf( BUFFSIZE );

    while ( blob.good() and retVal ) {
      blob.read( reinterpret_cast< char * >( buf.data() ), buf.size() ); // NOLINT
      size_t readcount = blob.gcount();
      retVal = call_back( reinterpret_cast< const char * >( buf.data() ), readcount ); // NOLINT
    }
  }

  return retVal;
}

auto OCI::Extensions::Dir::hasBlob( const Schema1::ImageManifest& im, SHA256 sha ) -> bool {
  (void)im;
  (void)sha;

  std::cerr << "OCI::Extensions::Dir::hasBlob Schema1::ImageManifest is not implemented" << std::endl;

  return false;
}

auto OCI::Extensions::Dir::hasBlob( Schema2::ImageManifest const& im, std::string const& target, SHA256 sha ) -> bool {
  bool retVal         = false;
  auto dir_map        = dirMap();

  if ( dir_map.find( im.name ) != dir_map.end() and dir_map.at( im.name ).path.find( im.requestedDigest ) != dir_map.at( im.name ).path.end() ) {
    // The map wins
    retVal = true;
  } else if ( not im.originDomain.empty() ) {
    auto blob_file      = _directory.path() / "blobs" / sha;
    auto image_dir_path = std::filesystem::directory_entry( _directory.path() / im.originDomain / ( im.name + ":" + im.requestedTarget ) / target);
    auto image_path     = image_dir_path.path() / sha;

    if ( std::filesystem::exists( image_path ) ) {
      retVal = validateFile( sha, image_path );
    } else if ( std::filesystem::exists( blob_file ) ) {
      if ( not image_dir_path.exists() ) {
        std::filesystem::create_directories( image_dir_path );
      }

      std::filesystem::create_symlink( blob_file, image_path );

      retVal = std::filesystem::exists( image_path );
    }
  }

  return retVal;
}

auto OCI::Extensions::Dir::putBlob( const Schema1::ImageManifest& im, const std::string& target, std::uintmax_t total_size, const char * blob_part, uint64_t blob_part_size ) -> bool {
  (void)im;
  (void)target;
  (void)total_size;
  (void)blob_part;
  (void)blob_part_size;

  std::cerr << "OCI::Extensions::Dir::putBlob Schema1::ImageManifest is not implemented" << std::endl;

  return false;
}

auto OCI::Extensions::Dir::putBlob( Schema2::ImageManifest const& im,
                                    std::string const&            target,
                                    SHA256 const&                 blob_sha,
                                    std::uintmax_t                total_size,
                                    const char*                   blob_part,
                                    uint64_t                      blob_part_size ) -> bool {
  auto retVal         = false;
  auto blob_dir       = std::filesystem::directory_entry( _directory.path() / "blobs" );
  auto temp_dir       = std::filesystem::directory_entry( _directory.path() / "temp" );
  auto image_dir_path = std::filesystem::directory_entry( _directory.path() / im.originDomain / ( im.name + ":" + im.requestedTarget ) / target );
  auto image_path     = image_dir_path.path() / blob_sha;
  auto blob_path      = blob_dir.path() /blob_sha;

  if ( _temp_file.empty() ) {
    _temp_file = temp_dir.path() / genUUID();
  }

  if ( not image_dir_path.exists() ) {
    std::lock_guard< std::mutex > lg( DIR_MUTEX );

    if ( not image_dir_path.exists() ) {
      std::filesystem::create_directories( image_dir_path );
    }
  }

  if ( not blob_dir.exists() ) {
    std::lock_guard< std::mutex > lg( DIR_MUTEX );

    if ( not blob_dir.exists() ) {
      std::filesystem::create_directories( blob_dir );
    }
  }

  // Temp dir may require external periodic cleaning
  //  or just be smart about it here
  //  Largest potential file size / slowest down load speed = oldest possible file
  if ( not temp_dir.exists() ) {
    std::lock_guard< std::mutex > lg( DIR_MUTEX );

    if ( not temp_dir.exists() ) {
      std::filesystem::create_directories( temp_dir );
    }
  }

  {// scoped so the file closes prior to any other operation
    std::ofstream blob( _temp_file, std::ios::app | std::ios::binary );
    
    retVal = blob.write( blob_part, blob_part_size ).good();
  }

  if ( retVal ) {
    _bytes_written += blob_part_size;
  }

  if ( _bytes_written == total_size ) {
    if ( validateFile( blob_sha, _temp_file ) ) {
      std::lock_guard< std::mutex > lg( DIR_MUTEX );

      if ( not std::filesystem::exists( blob_path ) ) {
        std::filesystem::copy_file( _temp_file, blob_path );
      }

      if ( std::filesystem::exists( blob_path ) ) {
        std::filesystem::create_symlink( blob_path, image_path );
      } else {
        std::cout << "Failed to copy file '" << _temp_file.string() << "' -> '" << blob_path.string() << "'" << std::endl;
      }

      std::filesystem::remove( _temp_file );
    } else if ( std::filesystem::remove( _temp_file ) ) {
      std::cerr << "Removed dirty file, file did not validate" << std::endl;
      std::filesystem::remove( _temp_file );

      retVal = false;
    }

    _temp_file = "";
  }

  return retVal; // FIXME: Need to return based on the disk write, is disk full, permission denied, or other issue
}

void OCI::Extensions::Dir::fetchManifest( Schema1::ImageManifest& im, Schema1::ImageManifest const& request ) {
  (void)im;
  (void)request;

  std::cerr << "OCI::Extensions::Dir::fetchManifest Schema1::ImageManifest is not implemented" << std::endl;
}

void OCI::Extensions::Dir::fetchManifest( Schema1::SignedImageManifest& sim, Schema1::SignedImageManifest const& request ) {
  (void)sim;
  (void)request;


  std::cerr << "OCI::Extensions::Dir::fetchManifest Schema1::SignedImageManifest is not implemented" << std::endl;
}

void OCI::Extensions::Dir::fetchManifest( Schema2::ManifestList& ml, Schema2::ManifestList const& request ) {
  // Expectation is we are in <base>/<domain> dir with a sub-tree of <repo>:<tags>
  auto dir_map = dirMap();

  if ( dir_map.find( request.name ) != dir_map.end() and dir_map.at( request.name ).path.find( request.requestedTarget ) != dir_map.at( request.name ).path.end() ) {
    auto ml_file_path  = dir_map.at( request.name ).path.at( request.requestedTarget ).path() / "ManifestList.json";
    auto ver_file_path = dir_map.at( request.name ).path.at( request.requestedTarget ).path() / "Version";

    if ( std::filesystem::exists( ver_file_path ) ) {
      std::ifstream ver_file( ver_file_path );
      ver_file >> ml.schemaVersion;

      if ( ml.schemaVersion == 2 and std::filesystem::exists( ml_file_path ) ) {
        nlohmann::json ml_json;
        std::ifstream ml_file( ml_file_path );
        ml_file >> ml_json;

        ml_json.get_to( ml );

        ml_json[ "originDomain" ].get_to( ml.originDomain );
        ml_json[ "requestedTarget" ].get_to( ml.requestedTarget );
        ml_json[ "name" ].get_to( ml.name );
      }
    }
  }
}

void OCI::Extensions::Dir::fetchManifest( Schema2::ImageManifest& im, Schema2::ImageManifest const& request ) {
  auto dir_map = dirMap();

  if ( dir_map.find( request.name ) != dir_map.end() and dir_map.at( request.name ).path.find( request.requestedDigest ) != dir_map.at( request.name ).path.end() ) {
    auto im_file_path = dir_map.at( request.name ).path.at( request.requestedDigest ).path() / "ImageManifest.json";

    if ( std::filesystem::exists( im_file_path ) ) {
      nlohmann::json im_json;
      std::ifstream im_file( im_file_path );
      im_file >> im_json;

      im_json.get_to( im );

      im_json[ "originDomain" ].get_to( im.originDomain );
      im_json[ "requestedTarget" ].get_to( im.requestedTarget );
      im_json[ "requestedDigest" ].get_to( im.requestedDigest );
      im_json[ "name" ].get_to( im.name );
    }
  } else {
    std::cerr << "OCI::Extensions::Dir::fetchManifest Unable to locate ImageManifest for " << request.name << "->" << request.requestedTarget << std::endl;
  }
}

auto OCI::Extensions::Dir::putManifest( Schema1::ImageManifest const& im, std::string const& target ) -> bool {
  bool retVal = true;
  (void)im;
  (void)target;

  std::cerr << "OCI::Extensions::Dir::putManifest Schema1::ImageManifest is not implemented" << std::endl;

  return retVal;
}

auto OCI::Extensions::Dir::putManifest( Schema1::SignedImageManifest const& sim, std::string const& target ) -> bool {
  bool retVal = true;
  (void)sim;
  (void)target;

  std::cerr << "OCI::Extensions::Dir::putManifest Schema1::SignedImageManifest is not implemented" << std::endl;

  return retVal;
}

auto OCI::Extensions::Dir::putManifest( Schema2::ManifestList const& ml, std::string const& target ) -> bool {
  (void)target;
  bool retVal                 = true;
  auto manifest_list_dir_path = std::filesystem::directory_entry( _directory.path() / ml.originDomain / ( ml.name + ":" + ml.requestedTarget ) );
  auto manifest_list_path     = manifest_list_dir_path.path() / "ManifestList.json";
  auto version_path           = manifest_list_dir_path.path() / "Version";


  nlohmann::json manifest_list_json = ml;

  // Output the Extensions for later reads
  manifest_list_json[ "originDomain" ]    = ml.originDomain;
  manifest_list_json[ "requestedTarget" ] = ml.requestedTarget;
  manifest_list_json[ "name" ]            = ml.name;

  bool complete = true;

  for ( auto const& im : ml.manifests ) {
    auto im_path = manifest_list_dir_path.path() / im.digest / "ImageManifest.json";

    if ( not std::filesystem::exists( im_path ) ) {
      complete = false;
      break;
    }
  }

  if ( complete ) {
    std::ofstream manifest_list( manifest_list_path ); 
    std::ofstream version( version_path );

    manifest_list << std::setw( 2 ) << manifest_list_json;
    version       << ml.schemaVersion;
  }

  return retVal;
}

auto OCI::Extensions::Dir::putManifest( Schema2::ImageManifest const& im, std::string& target ) -> bool {
  bool retVal              = true;
  auto image_dir_path      = std::filesystem::directory_entry( _directory.path() / im.originDomain / ( im.name + ":" + im.requestedTarget ) / target );
  auto image_manifest_path = image_dir_path.path() / "ImageManifest.json";

  nlohmann::json image_manifest_json = im;

  // Output the Extensions for later reads
  image_manifest_json[ "originDomain" ]    = im.originDomain;
  image_manifest_json[ "requestedTarget" ] = im.requestedTarget;
  image_manifest_json[ "requestedDigest" ] = im.requestedDigest;
  image_manifest_json[ "name" ]            = im.name;

  for ( auto const& layer : im.layers ) {
    if ( not hasBlob( im, target, layer.digest ) ) {
      retVal = false;
      break;
    }
  }

  if ( not hasBlob( im, target, im.config.digest ) ) {
    std::cerr << "Config digest missing for " << im.name << ":" << im.requestedTarget << "/" << im.requestedDigest << std::endl;
    retVal = false;
  }

  if ( retVal ) {
    std::ofstream image_manifest( image_manifest_path );

    image_manifest << std::setw( 2 ) << image_manifest_json;
  }

  return retVal;
}

auto OCI::Extensions::Dir::tagList( std::string const& rsrc ) -> OCI::Tags {
  OCI::Tags retVal;
  auto dir_map = dirMap();

  if ( dir_map.empty() ) {
    catalog();
  }

  if ( dir_map.find( rsrc ) != dir_map.end() ) {
    retVal.name = rsrc;
    retVal.tags = dir_map.at( rsrc ).tags;
  }

  return retVal;
}

auto OCI::Extensions::Dir::dirMap() -> DirMap const& {
  static std::map< std::string, DirMap > retVal;
  auto dir = _directory;

  // expecting dir to be root of the tree
  //  - subtree of namespaces
  //   - subtree of repo-name:(tag|digest) (contents varies based on Schema version)
  //    - subtree of digest with ImageManifests and blobs (Schemav2 only)
  // Top level can be either the root dir with domain subtrees or within a domain dir as tree root

  if ( retVal[ _directory.path().string() ].empty() ) {
    std::lock_guard< std::mutex > lg( DIR_MAP_MUT );

    if ( retVal[ _directory.path().string() ].empty() ) {
      std::cout << "Generating Directory Map of: " << _directory.path().string() << std::endl;
      auto &dir_map = retVal[ _directory.path().string() ];
      auto base_dir = dir;

      for ( auto const& path_part : std::filesystem::recursive_directory_iterator( dir ) ) {
        if ( path_part.is_directory() and path_part.path().parent_path() == dir and path_part.path().filename().string().find( '.' ) != std::string::npos ) { // Will not work with shortnames, requires fqdn
          base_dir = path_part; // This is to account for the domain case, for a Docker -> Dir, where we may already have images
        } else if ( path_part.path().string().find( ':' ) != std::string::npos ) {
          auto repo_str = path_part.path().string().substr( base_dir.path().string().size() + 1 ); // + 1 to remove trailing slash

          if ( path_part.is_directory() ) {
            if ( std::count( repo_str.begin(), repo_str.end(), '/' ) == 1 ) {
              auto tag       = repo_str.substr( repo_str.find( ':' ) + 1 );
              auto repo_name = repo_str.substr( 0, repo_str.find( ':' ) );

              dir_map[ repo_name ].tags.push_back( tag );
              dir_map[ repo_name ].path[ tag ] = path_part;
            } else {
              auto repo_name = repo_str.substr( 0, repo_str.find( ':' ) );
              auto target    = repo_str.substr( repo_str.find_last_of( '/' ) + 1 );

              dir_map[ repo_name ].path[ target ] = path_part;
            }
          } else if ( repo_str.find( "sha" ) != std::string::npos and repo_str.find( "json" ) == std::string::npos ) {
            // Blobs
            auto repo_name = repo_str.substr( 0, repo_str.find( ':' ) );
            auto target    = repo_str.substr( repo_str.find_last_of( '/' ) + 1 );

            dir_map[ repo_name ].path[ target ] = path_part;
          }
        }
      }
    }
  }

  return retVal[ _directory.path().string() ];
}
