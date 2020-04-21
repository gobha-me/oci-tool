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
      explicit Dir( std::string const & directory ); // this would be the base path
      ~Dir() override = default;

      void fetchBlob( const std::string& rsrc, SHA256 sha, std::function< bool(const char *, uint64_t ) >& call_back ) override; // To where

      auto hasBlob( const Schema1::ImageManifest& im, SHA256 sha ) -> bool override;
      auto hasBlob( const Schema2::ImageManifest& im, const std::string& target, SHA256 sha ) -> bool override;

      void putBlob( const Schema1::ImageManifest& im, const std::string& target, std::uintmax_t total_size, const char * blob_part, uint64_t blob_part_size ) override;
      void putBlob( const Schema2::ImageManifest& im, const std::string& target, const SHA256& blob_sha, std::uintmax_t total_size, const char * blob_part, uint64_t blob_part_size ) override;

      void inspect( std::string rsrc );

      void fetchManifest( Schema1::ImageManifest& im,        const std::string& rsrc, const std::string& target ) override;
      void fetchManifest( Schema1::SignedImageManifest& sim, const std::string& rsrc, const std::string& target ) override;
      void fetchManifest( Schema2::ManifestList& ml,         const std::string& rsrc, const std::string& target ) override;
      void fetchManifest( Schema2::ImageManifest& im,        const std::string& rsrc, const std::string& target ) override;

      void putManifest( const Schema1::ImageManifest& im,        const std::string& rsrc, const std::string& target ) override;
      void putManifest( const Schema1::SignedImageManifest& sim, const std::string& rsrc, const std::string& target ) override;
      void putManifest( const Schema2::ManifestList& ml,         const std::string& rsrc, const std::string& target ) override;
      void putManifest( const Schema2::ImageManifest& im,        const std::string& rsrc, const std::string& target ) override;

      auto tagList( const std::string& rsrc ) -> Tags override;
    protected:
    private:
      std::filesystem::directory_entry _directory;
  };
} // namespace OCI::Extensions
