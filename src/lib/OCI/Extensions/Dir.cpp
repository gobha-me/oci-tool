#include <OCI/Extensions/Dir.hpp>
#include <filesystem>
#include <iostream>

OCI::Extensions::Dir::Dir() = default;
OCI::Extensions::Dir::Dir( std::string const& directory ) : _directory( directory ) {
  if ( not _directory.exists() ) { // TODO: do we create, this 'root/base' dir or fail if it doesn't exist?
    std::filesystem::create_directories( _directory );
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

auto OCI::Extensions::Dir::hasBlob( const Schema2::ImageManifest& im, SHA256 sha ) -> bool {
  (void)im;
  (void)sha;

  std::cout << "OCI::Extensions::Dir::hasBlob Schema2::ImageManifest is not implemented" << std::endl;

  return false;
}

void OCI::Extensions::Dir::putBlob( const Schema1::ImageManifest& im, const std::string& target, std::uintmax_t total_size, const char * blob_part, uint64_t blob_part_size ) {
  (void)im;
  (void)target;
  (void)total_size;
  (void)blob_part;
  (void)blob_part_size;

  std::cout << "OCI::Extensions::Dir::putBlob Schema1::ImageManifest is not implemented" << std::endl;
}

void OCI::Extensions::Dir::putBlob( const Schema2::ImageManifest& im,
                                    const std::string& target,
                                    const SHA256& blob_sha,
                                    std::uintmax_t total_size,
                                    const char * blob_part,
                                    uint64_t blob_part_size ) {
  (void)im;
  (void)target;
  (void)total_size;
  (void)blob_part;
  (void)blob_part_size;

  if ( im.requestedTarget.empty() ) {
    std::abort();
  }

  std::cout << "OCI::Extensions::Dir::putBlob Schema2::ImageManifest is not implemented" << std::endl;
  std::cout << im.originDomain << "/" << im.name << ":" << im.requestedTarget << "/" << target << " " << total_size << " " << blob_part_size << std::endl;
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
