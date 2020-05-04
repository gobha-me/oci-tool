#include <OCI/Extensions/Dir.hpp>
#include <botan/hash.h>
#include <botan/hex.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <mutex>
#include <set>

std::mutex DIR_MUTEX;

OCI::Extensions::Dir::Dir() = default;
OCI::Extensions::Dir::Dir( std::string const& directory ) {
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
  auto const& dir_map = dirMap();

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

auto OCI::Extensions::Dir::hasBlob( const Schema2::ImageManifest& im, const std::string& target, SHA256 sha ) -> bool {
  bool retVal         = false;
  auto const& dir_map = dirMap();
  std::filesystem::path image_path;

  if ( dir_map.empty() ) {
    image_path = _directory.path() / im.originDomain / ( im.name + ":" + im.requestedTarget ) / target / sha;
  } else if ( dir_map.find( im.name ) != dir_map.end() and dir_map.at( im.name ).path.find( target ) != dir_map.at( im.name ).path.end() ) {
    image_path = dir_map.at( im.name ).path.at( target ).path() / sha;
  }

  if ( std::filesystem::exists( image_path ) ) {
    constexpr auto BUFFSIZE = 2048;
    auto sha256( Botan::HashFunction::create( "SHA-256" ) );
    std::ifstream blob( image_path, std::ios::binary );
    std::vector< uint8_t > buf( BUFFSIZE );

    while ( blob.good() ) {
      blob.read( reinterpret_cast< char * >( buf.data() ), buf.size() ); // NOLINT
      size_t readcount = blob.gcount();
      sha256->update( buf.data(), readcount );
    }

    std::string sha256_str = Botan::hex_encode( sha256->final() );
    std::for_each( sha256_str.begin(), sha256_str.end(), []( char & c ) {
        c = std::tolower( c ); // NOLINT - narrowing warning, but unsigned char for the lambda doesn't build, so which is it
      } );

    if ( sha == "sha256:" + sha256_str ) {
      retVal = true;
    } else {
      // remove a failed file, as the write point only appends, and an incomplete or bad file could cause problems
      std::filesystem::remove( image_path );
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
  (void)total_size;
  auto image_dir_path = std::filesystem::directory_entry( _directory.path() / im.originDomain / ( im.name + ":" + im.requestedTarget ) / target );
  auto image_path     = image_dir_path.path() / blob_sha;

  if ( not image_dir_path.exists() ) {
    std::lock_guard< std::mutex > lg( DIR_MUTEX );

    if ( not image_dir_path.exists() ) {
      std::filesystem::create_directories( image_dir_path );
    }
  }

  std::ofstream blob( image_path, std::ios::app | std::ios::binary );
  
  blob.write( blob_part, blob_part_size );

  return true; // FIXME: Need to return based on the disk write, is disk full, permission denied, or other issue
}

void OCI::Extensions::Dir::fetchManifest( Schema1::ImageManifest& im, const std::string& rsrc, const std::string& target ) {
  (void)im;
  (void)rsrc;
  (void)target;

  std::cerr << "OCI::Extensions::Dir::fetchManifest Schema1::ImageManifest is not implemented" << std::endl;
}

void OCI::Extensions::Dir::fetchManifest( Schema1::SignedImageManifest& sim, const std::string& rsrc, const std::string& target ) {
  (void)sim;
  (void)rsrc;
  (void)target;

  std::cerr << "OCI::Extensions::Dir::fetchManifest Schema1::SignedImageManifest is not implemented" << std::endl;
}

void OCI::Extensions::Dir::fetchManifest( Schema2::ManifestList& ml, const std::string& rsrc, const std::string& target ) {
  // Expectation is we are in <base>/<domain> dir with a sub-tree of <repo>:<tags>
  auto const& dir_map = dirMap();

  if ( dir_map.find( rsrc ) != dir_map.end() and dir_map.at( rsrc ).path.find( target ) != dir_map.at( rsrc ).path.end() ) {
    auto ml_file_path  = dir_map.at( rsrc ).path.at( target ).path() / "ManifestList.json";
    auto ver_file_path = dir_map.at( rsrc ).path.at( target ).path() / "Version";

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

void OCI::Extensions::Dir::fetchManifest( Schema2::ImageManifest& im, std::string const& rsrc, std::string const& target ) {
  auto const& dir_map = dirMap();

  if ( dir_map.find( rsrc ) != dir_map.end() and dir_map.at( rsrc ).path.find( target ) != dir_map.at( rsrc ).path.end() ) {
    auto im_file_path = dir_map.at( rsrc ).path.at( target ).path() / "ImageManifest.json";

    if ( std::filesystem::exists( im_file_path ) ) {
      nlohmann::json im_json;
      std::ifstream im_file( im_file_path );
      im_file >> im_json;

      im_json.get_to( im );

      im_json[ "originDomain" ].get_to( im.originDomain );
      im_json[ "requestedTarget" ].get_to( im.requestedTarget );
      im_json[ "name" ].get_to( im.name );
    }
  } else {
    std::cerr << "Unable to locate ImagManifest for " << rsrc << "->" << target << std::endl;
  }
}

auto OCI::Extensions::Dir::putManifest( const Schema1::ImageManifest& im, const std::string& target ) -> bool {
  bool retVal = true;
  (void)im;
  (void)target;

  std::cerr << "OCI::Extensions::Dir::putManifest Schema1::ImageManifest is not implemented" << std::endl;

  return retVal;
}

auto OCI::Extensions::Dir::putManifest( const Schema1::SignedImageManifest& sim, const std::string& target ) -> bool {
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
  image_manifest_json[ "name" ]            = im.name;

  //TODO: validate each blob prior to write (existance and sha256 is correct)

  bool complete = true;

  for ( auto const& layer : im.layers ) {
    if ( not hasBlob( im, target, layer.digest ) ) {
      complete = false;
      break;
    }
  }

  if ( complete ) {
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

std::mutex DIR_MAP_MUT;
auto OCI::Extensions::Dir::dirMap() -> DirMap const& {
  static DirMap retVal;
  auto dir = _directory;

  // expecting dir to be root of the tree
  //  - subtree of namespaces
  //   - subtree of repo-name:(tag|digest) (contents varies based on Schema version)
  //    - subtree of digest with ImageManifests and blobs (Schemav2 only)
  // Top level can be either the root dir with domain subtrees or within a domain dir as tree root

  if ( retVal.empty() ) {
    std::lock_guard< std::mutex > lg( DIR_MAP_MUT );

    if ( retVal.empty() ) {
      auto base_dir = dir;
      for ( auto const& path_part : std::filesystem::recursive_directory_iterator( dir ) ) {
        if ( path_part.is_directory() and path_part.path().filename().string().find( '.' ) != std::string::npos ) { // Will not work with shortnames, requires fqdn
          base_dir = path_part; // This is to account for the domain case, for a Docker -> Dir, where we may already have images
        } else if ( path_part.path().string().find( ':' ) != std::string::npos ) {
          auto repo_str = path_part.path().string().substr( base_dir.path().string().size() + 1 ); // + 1 to remove trailing slash

          if ( path_part.is_directory() ) {
            if ( std::count( repo_str.begin(), repo_str.end(), '/' ) == 1 ) {
              auto tag       = repo_str.substr( repo_str.find( ':' ) + 1 );
              auto repo_name = repo_str.substr( 0, repo_str.find( ':' ) );

              retVal[ repo_name ].tags.push_back( tag );
              retVal[ repo_name ].path[ tag ] = path_part;
            } else {
              auto repo_name = repo_str.substr( 0, repo_str.find( ':' ) );
              auto target    = repo_str.substr( repo_str.find_last_of( '/' ) + 1 );

              retVal[ repo_name ].path[ target ] = path_part;
            }
          } else if ( repo_str.find( "sha" ) != std::string::npos ) {
            // Blobs
            auto repo_name = repo_str.substr( 0, repo_str.find( ':' ) );
            auto target    = repo_str.substr( repo_str.find_last_of( '/' ) + 1 );

            retVal[ repo_name ].path[ target ] = path_part;
          }
        }
      }
    }
  }

  return retVal;
}
