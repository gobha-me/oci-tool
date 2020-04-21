#include <OCI/Extensions/Dir.hpp>
#include <filesystem>
#include <iostream>
#include <fstream>

OCI::Extensions::Dir::Dir() = default;
OCI::Extensions::Dir::Dir( std::string const& directory ) : _directory( directory ) {
  if ( not _directory.exists() ) { // TODO: do we create, this 'root/base' dir or fail if it doesn't exist?
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
  auto image_path = _directory.path() / im.originDomain / ( im.name + ":" + im.requestedTarget ) / target / sha;

  // exists is not quite enough, it should also match its digest to be true
  return std::filesystem::exists( image_path );
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

void OCI::Extensions::Dir::putManifest( const Schema1::ImageManifest& im, const std::string& rsrc, const std::string& target ) {
  (void)im;
  (void)rsrc;
  (void)target;

  std::cout << "OCI::Extensions::Dir::putManifest Schema1::ImageManifest is not implemented" << std::endl;
}

void OCI::Extensions::Dir::putManifest( const Schema1::SignedImageManifest& sim, const std::string& rsrc, const std::string& target ) {
  (void)sim;
  (void)rsrc;
  (void)target;

  std::cout << "OCI::Extensions::Dir::putManifest Schema1::SignedImageManifest is not implemented" << std::endl;
}

void OCI::Extensions::Dir::putManifest( const Schema2::ManifestList& ml, const std::string& rsrc, const std::string& target ) {
  (void)ml;
  (void)rsrc;
  (void)target;

  std::cout << "OCI::Extensions::Dir::putManifest Schema2::ManifestList is not implemented" << std::endl;
}

void OCI::Extensions::Dir::putManifest( const Schema2::ImageManifest& im, const std::string& rsrc, const std::string& target ) {
  (void)im;
  (void)rsrc;
  (void)target;

  std::cout << "OCI::Extensions::Dir::putManifest Schema2::ImageManifest is not implemented" << std::endl;
}

auto OCI::Extensions::Dir::tagList( const std::string& rsrc ) -> OCI::Tags {
  OCI::Tags retVal;
  (void)rsrc;

  std::cout << "OCI::Extensions::Dir::tagList is not implemented" << std::endl;

  return retVal;
}
