#pragma once
#include <OCI/Schema1.hpp>
#include <OCI/Schema2.hpp>
#include <filesystem>

namespace OCI {
  class Dir {
    public:
      Dir( std::string const & directory ); // this would be the base path
      ~Dir();

      void inspect( std::string rsrc );

      template< class Schema_t >
      Schema_t manifest( std::string rsrc );

      void pull( Schema1::ImageManifest );
      void pull( Schema2::ManifestList );

      void push( Schema1::ImageManifest );
      void push( Schema2::ManifestList );

      bool pingResource( std::string rsrc );
    protected:
    private:
      std::filesystem::directory_entry _directory;
  };
}

// IMPLEMENTATION
// OCI::Dir
OCI::Dir::Dir( std::string const& directory ) : _directory( directory ) {
  if ( not _directory.exists() )
    std::filesystem::create_directories( _directory );
}
OCI::Dir::~Dir() = default;
