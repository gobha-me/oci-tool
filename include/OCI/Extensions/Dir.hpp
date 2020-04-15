#pragma once
#include <OCI/Schema1.hpp>
#include <OCI/Schema2.hpp>
#include <filesystem>

namespace OCI::Extensions {
  class Dir {
    public:
      Dir();
      Dir( std::string const & directory ); // this would be the base path
      ~Dir();

      void inspect( std::string rsrc );

      template< class Schema_t >
      Schema_t manifest( std::string rsrc );

      void pull( Schema1::ImageManifest im );
      void pull( Schema2::ManifestList ml );
      void pull( Schema2::ImageManifest im );

      void push( Schema1::ImageManifest im );
      void push( Schema2::ManifestList ml );

      bool pingResource( std::string rsrc );
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
