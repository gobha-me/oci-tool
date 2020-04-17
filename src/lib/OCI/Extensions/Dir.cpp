#include <OCI/Extensions/Dir.hpp>
#include <iostream>

OCI::Extensions::Dir::Dir() = default;
OCI::Extensions::Dir::Dir( std::string const& directory ) : _directory( directory ) {
  if ( not _directory.exists() ) {
    std::filesystem::create_directories( _directory );
  }
}

void OCI::Extensions::Dir::fetchBlob( const std::string& rsrc, SHA256 sha, std::function< void(const char *, uint64_t ) >& call_back ) {
  (void)rsrc;
  (void)sha;

  std::cout << "OCI::Extensions::Dir::fetchBlob is not implemented" << std::endl;

  call_back( "hello world", 12 ); // NOLINT
}

auto OCI::Extensions::Dir::hasBlob( const std::string& rsrc, SHA256 sha ) -> bool {
  (void)rsrc;
  (void)sha;

  std::cout << "OCI::Extensions::Dir::hasBlob is not implemented" << std::endl;

  return false;
}

void OCI::Extensions::Dir::manifest( Schema1::ImageManifest& im, const std::string& rsrc, const std::string& target ) {
  (void)im;
  (void)rsrc;
  (void)target;

  std::cout << "OCI::Extensions::Dir::manifest is not implemented" << std::endl;
}
void OCI::Extensions::Dir::manifest( Schema1::SignedImageManifest& sim, const std::string& rsrc, const std::string& target ) {
  (void)sim;
  (void)rsrc;
  (void)target;

  std::cout << "OCI::Extensions::Dir::manifest is not implemented" << std::endl;
}
void OCI::Extensions::Dir::manifest( Schema2::ManifestList& ml, const std::string& rsrc, const std::string& target ) {
  (void)ml;
  (void)rsrc;
  (void)target;
}
void OCI::Extensions::Dir::manifest( Schema2::ImageManifest& im, const std::string& rsrc, const std::string& target ) {
  (void)im;
  (void)rsrc;
  (void)target;

  std::cout << "OCI::Extensions::Dir::manifest is not implemented" << std::endl;
}

void OCI::Extensions::Dir::putBlob( const std::string& rsrc, const std::string& target, std::uintmax_t total_size, const char * blob_part, uint64_t blob_part_size ) {
  (void)rsrc;
  (void)target;
  (void)total_size;
  (void)blob_part;
  (void)blob_part_size;

  std::cout << "OCI::Extensions::Dir::putBlob is not implemented" << std::endl;
}

auto OCI::Extensions::Dir::tagList( const std::string& rsrc ) -> OCI::Tags {
  OCI::Tags retVal;
  (void)rsrc;

  std::cout << "OCI::Extensions::Dir::tagList is not implemented" << std::endl;

  return retVal;
}
