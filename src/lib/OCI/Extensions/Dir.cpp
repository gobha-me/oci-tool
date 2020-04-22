#include <OCI/Extensions/Dir.hpp>
#include <botan/hash.h>
#include <botan/hex.h>
#include <filesystem>
#include <iostream>
#include <fstream>

OCI::Extensions::Dir::Dir() = default;
OCI::Extensions::Dir::Dir( std::string const& directory ) : _directory( directory ) {
  if ( not _directory.exists() ) {
    std::cerr << _directory.path() << " does not exist." << std::endl;
    std::abort();
  }
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
  (void)ml;
  (void)rsrc;
  (void)target;

  std::cout << "OCI::Extensions::Dir::fetchManifest Schema2::ManifestList is not implemented" << std::endl;
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

    manifest_list << manifest_list_json.dump( 2 );
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

    image_manifest << image_manifest_json.dump( 2 );
  }
}

auto OCI::Extensions::Dir::tagList( std::string const& rsrc ) -> OCI::Tags {
  OCI::Tags retVal;
  (void)rsrc;

  std::cout << "OCI::Extensions::Dir::tagList is not implemented" << std::endl;

  return retVal;
}
