#include <OCI/Extensions/Dir.hpp>
#include <botan/hash.h>
#include <botan/hex.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <set>

OCI::Extensions::Dir::Dir() = default;
OCI::Extensions::Dir::Dir( std::string const& directory ) {
  auto dir            = directory;
  auto trailing_slash = dir.find_last_of( '/' );

  if ( trailing_slash == dir.size() - 1 ) {
    _directory = std::filesystem::directory_entry( dir.substr( 0, trailing_slash ) );
  } else {
    _directory = std::filesystem::directory_entry( dir );
  }

  if ( not ( _directory.exists() or _directory.is_directory() ) ) {
    std::cerr << _directory.path() << " does not exist or is not a directory." << std::endl;
    std::abort();
  }
}

auto OCI::Extensions::Dir::catalog() -> OCI::Catalog {
  Catalog retVal;

  // expecting dir to be root of the tree
  //  - subtree of namespaces
  //   - subtree of repo-name:(tag|digest)

  std::set< std::string > repos;
  
  for ( auto const& path_part : std::filesystem::recursive_directory_iterator( _directory ) ) {
    if ( path_part.path().parent_path().compare( _directory.path() ) != 0 ) {
      if ( path_part.is_directory() ) {
        auto repo_str = path_part.path().string().substr( _directory.path().string().size() + 1 );

        if ( std::count( repo_str.begin(), repo_str.end(), '/' ) == 1 ) {
          auto tag = repo_str.substr( repo_str.find( ':' ) + 1 );
          repo_str = repo_str.substr( 0, repo_str.find( ':' ) );

          repos.insert( repo_str );
          _tags[ repo_str ].tags.push_back( tag );
        }
      }
    }
  }

  for ( auto const& repo : repos ) {
    retVal.repositories.push_back( repo );
  }

  return retVal;
}

void OCI::Extensions::Dir::fetchBlob( const std::string& rsrc, SHA256 sha, std::function< bool(const char *, uint64_t ) >& call_back ) {
  (void)rsrc;
  (void)sha;

  std::cout << "OCI::Extensions::Dir::fetchBlob is not implemented" << std::endl;

  call_back( "hello world", 12 ); // NOLINT
}

auto OCI::Extensions::Dir::hasBlob( const Schema1::ImageManifest& im, SHA256 sha ) -> bool {
  (void)im;
  (void)sha;

  std::cout << "OCI::Extensions::Dir::hasBlob Schema1::ImageManifest is not implemented" << std::endl;

  return false;
}

auto OCI::Extensions::Dir::hasBlob( const Schema2::ImageManifest& im, const std::string& target, SHA256 sha ) -> bool {
  bool retVal     = false;
  auto image_path = _directory.path() / im.originDomain / ( im.name + ":" + im.requestedTarget ) / target / sha;

  if ( std::filesystem::exists( image_path ) ) {
    auto sha256( Botan::HashFunction::create( "SHA-256" ) );
    std::ifstream blob( image_path, std::ios::binary );
    std::vector< uint8_t > buf( 2048 );

    while ( blob.good() ) {
      blob.read( reinterpret_cast< char * >( buf.data() ), buf.size() );
      size_t readcount = blob.gcount();
      sha256->update( buf.data(), readcount );
    }

    std::string sha256_str = Botan::hex_encode( sha256->final() );
    std::for_each( sha256_str.begin(), sha256_str.end(), []( char & c ) {
        c = std::tolower( c );
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

void OCI::Extensions::Dir::putBlob( const Schema1::ImageManifest& im, const std::string& target, std::uintmax_t total_size, const char * blob_part, uint64_t blob_part_size ) {
  (void)im;
  (void)target;
  (void)total_size;
  (void)blob_part;
  (void)blob_part_size;

  std::cout << "OCI::Extensions::Dir::putBlob Schema1::ImageManifest is not implemented" << std::endl;
}

void OCI::Extensions::Dir::putBlob( Schema2::ImageManifest const& im,
                                    std::string const&            target,
                                    SHA256 const&                 blob_sha,
                                    std::uintmax_t                total_size,
                                    const char*                   blob_part,
                                    uint64_t                      blob_part_size ) {
  auto image_dir_path = std::filesystem::directory_entry( _directory.path() / im.originDomain / ( im.name + ":" + im.requestedTarget ) / target );
  auto image_path     = image_dir_path.path() / blob_sha;

  if ( not image_dir_path.exists() ) {
    std::filesystem::create_directories( image_dir_path );
  }

  std::ofstream blob( image_path, std::ios::app | std::ios::binary );
  
  blob.write( blob_part, blob_part_size );
}

void OCI::Extensions::Dir::fetchManifest( Schema1::ImageManifest& im, const std::string& rsrc, const std::string& target ) {
  (void)im;
  (void)rsrc;
  (void)target;

  std::cout << "OCI::Extensions::Dir::fetchManifest Schema1::ImageManifest is not implemented" << std::endl;
}

void OCI::Extensions::Dir::fetchManifest( Schema1::SignedImageManifest& sim, const std::string& rsrc, const std::string& target ) {
  (void)sim;
  (void)rsrc;
  (void)target;

  std::cout << "OCI::Extensions::Dir::fetchManifest Schema1::SignedImageManifest is not implemented" << std::endl;
}

void OCI::Extensions::Dir::fetchManifest( Schema2::ManifestList& ml, const std::string& rsrc, const std::string& target ) {
  // Expectation is we are in <base>/<domain> dir with a sub-tree of <repo>:<tags>
  auto ml_file_path  = _directory.path() / ( rsrc + ":" + target ) / "ManifestList.json";
  auto ver_file_path = _directory.path() / ( rsrc + ":" + target ) / "Version";

  if ( std::filesystem::exists( ver_file_path ) ) {
    std::ifstream ver_file( ver_file_path );
    ver_file >> ml.schemaVersion;

    if ( ml.schemaVersion == 2 and std::filesystem::exists( ml_file_path ) ) {
      nlohmann::json ml_json;
      std::ifstream ml_file( ml_file_path );
      ml_file >> ml_json;

      ml_json.get_to( ml );
    }
  }
  // when getting the manifest from disk the origDomain is in the dir name, how to get
}

void OCI::Extensions::Dir::fetchManifest( Schema2::ImageManifest& im, const std::string& rsrc, const std::string& target ) {
  (void)im;
  (void)rsrc;
  (void)target;

  std::cout << "OCI::Extensions::Dir::fetchManifest Schema2::ImageManifest is not implemented" << std::endl;
}

void OCI::Extensions::Dir::putManifest( const Schema1::ImageManifest& im, const std::string& target ) {
  (void)im;
  (void)target;

  std::cout << "OCI::Extensions::Dir::putManifest Schema1::ImageManifest is not implemented" << std::endl;
}

void OCI::Extensions::Dir::putManifest( const Schema1::SignedImageManifest& sim, const std::string& target ) {
  (void)sim;
  (void)target;

  std::cout << "OCI::Extensions::Dir::putManifest Schema1::SignedImageManifest is not implemented" << std::endl;
}

void OCI::Extensions::Dir::putManifest( Schema2::ManifestList const& ml, std::string const& target ) {
  auto manifest_list_dir_path = std::filesystem::directory_entry( _directory.path() / ml.originDomain / ( ml.name + ":" + ml.requestedTarget ) );
  auto manifest_list_path     = manifest_list_dir_path.path() / "ManifestList.json";
  auto version_path           = manifest_list_dir_path.path() / "Version";


  nlohmann::json manifest_list_json = ml;

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
}

void OCI::Extensions::Dir::putManifest( Schema2::ImageManifest const& im, std::string const& target ) {
  auto image_dir_path      = std::filesystem::directory_entry( _directory.path() / im.originDomain / ( im.name + ":" + im.requestedTarget ) / target );
  auto image_manifest_path = image_dir_path.path() / "ImageManifest.json";

  nlohmann::json image_manifest_json = im;

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
}

auto OCI::Extensions::Dir::tagList( std::string const& rsrc ) -> OCI::Tags {
  Tags retVal;

  if ( _tags.empty() ) {
    catalog();
  }

  if ( _tags.find( rsrc ) != _tags.end() ) {
    retVal = _tags.at( rsrc );
  }

  return retVal;
}
