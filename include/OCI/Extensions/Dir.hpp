#pragma once
#include <OCI/Base/Client.hpp>
#include <OCI/Schema1.hpp>
#include <OCI/Schema2.hpp>
#include <OCI/Tags.hpp>
#include <filesystem>

namespace OCI::Extensions {
  class Dir : public OCI::Base::Client {
    public:
      using SHA256 = std::string; // kinda preemptive, incase this becomes a real type
    public:
      Dir();
      Dir( std::string const & directory ); // this would be the base path
      ~Dir();

      void fetchBlob( SHA256 sha, std::function< void() >& call_back ); // To where

      bool hasBlob( SHA256 sha );

      void inspect( std::string rsrc );

      void manifest( Schema1::ImageManifest& im, const std::string& rsrc, const std::string& target );
      void manifest( Schema1::SignedImageManifest& sim, const std::string& rsrc, const std::string& target );
      void manifest( Schema2::ManifestList& ml, const std::string& rsrc, const std::string& target );
      void manifest( Schema2::ImageManifest& im, const std::string& rsrc, const std::string& target );

      Tags tagList( const std::string& rsrc );
    protected:
    private:
      std::filesystem::directory_entry _directory;
  };
} // namespace OCI::Extensions

// IMPLEMENTATION
// OCI::Dir
OCI::Extensions::Dir::Dir() {}
OCI::Extensions::Dir::Dir( std::string const& directory ) : _directory( directory ) {
  if ( not _directory.exists() )
    std::filesystem::create_directories( _directory );
}
OCI::Extensions::Dir::~Dir() = default;

void OCI::Extensions::Dir::fetchBlob( SHA256 sha, std::function< void() >& call_back ) {
  (void)sha;
  (void)call_back;
}

bool OCI::Extensions::Dir::hasBlob( SHA256 sha ) {
  (void)sha;
  return false;
}

void OCI::Extensions::Dir::manifest( Schema1::ImageManifest& im, const std::string& rsrc, const std::string& target ) {
  (void)im;
  (void)rsrc;
  (void)target;
}
void OCI::Extensions::Dir::manifest( Schema1::SignedImageManifest& sim, const std::string& rsrc, const std::string& target ) {
  (void)sim;
  (void)rsrc;
  (void)target;
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
}

OCI::Tags OCI::Extensions::Dir::tagList( const std::string& rsrc ) {
  OCI::Tags retVal;
  (void)rsrc;

  return retVal;
}
