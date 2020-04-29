#pragma once
#include <OCI/Base/Client.hpp>
#include <OCI/Schema1.hpp>
#include <OCI/Schema2.hpp>
#include <OCI/Tags.hpp>
#include <filesystem>

namespace OCI::Extensions {
  class Dir : public OCI::Base::Client {
    public:
      Dir();
      explicit Dir( std::string const & directory ); // this would be the base path
      ~Dir() override = default;

      auto catalog() -> OCI::Catalog override;

      auto fetchBlob( std::string const& rsrc, SHA256 sha, std::function< bool(const char *, uint64_t ) >& call_back ) -> bool override;

      auto hasBlob( Schema1::ImageManifest const& im, SHA256 sha ) -> bool override;
      auto hasBlob( Schema2::ImageManifest const& im, std::string const& target, SHA256 sha ) -> bool override;
                                                                       
      auto putBlob( Schema1::ImageManifest const& im, std::string const& target, std::uintmax_t total_size, const char * blob_part,    uint64_t blob_part_size ) -> bool override;
      auto putBlob( Schema2::ImageManifest const& im, std::string const& target, SHA256 const&  blob_sha,   std::uintmax_t total_size, const char * blob_part, uint64_t blob_part_size ) -> bool override;

      void fetchManifest( Schema1::ImageManifest& im,        std::string const& rsrc, std::string const& target ) override;
      void fetchManifest( Schema1::SignedImageManifest& sim, std::string const& rsrc, std::string const& target ) override;
      void fetchManifest( Schema2::ManifestList& ml,         std::string const& rsrc, std::string const& target ) override;
      void fetchManifest( Schema2::ImageManifest& im,        std::string const& rsrc, std::string const& target ) override;

      void putManifest( Schema1::ImageManifest const& im,        std::string const& target ) override;
      void putManifest( Schema1::SignedImageManifest const& sim, std::string const& target ) override;
      void putManifest( Schema2::ManifestList const& ml,         std::string const& target ) override;
      void putManifest( Schema2::ImageManifest const& im,        std::string const& target ) override;

      auto tagList( const std::string& rsrc ) -> Tags override;
    protected:
    private:
      std::filesystem::directory_entry _directory;
      std::map< std::string, Tags >    _tags;
      std::map< std::string,
        std::map< std::string, std::filesystem::directory_entry > > _dir_map;
  };
} // namespace OCI::Extensions
