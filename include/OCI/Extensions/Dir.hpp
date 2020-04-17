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

      void fetchBlob( const std::string& rsrc, SHA256 sha, std::function< void(const char *, uint64_t ) >& call_back ) override; // To where

      auto hasBlob( const std::string& rsrc, SHA256 sha ) -> bool override;

      void inspect( std::string rsrc );

      void manifest( Schema1::ImageManifest& im, const std::string& rsrc, const std::string& target ) override;
      void manifest( Schema1::SignedImageManifest& sim, const std::string& rsrc, const std::string& target ) override;
      void manifest( Schema2::ManifestList& ml, const std::string& rsrc, const std::string& target ) override;
      void manifest( Schema2::ImageManifest& im, const std::string& rsrc, const std::string& target ) override;

      void putBlob( const std::string& rsrc, const std::string& target, std::uintmax_t total_size, const char * blob_part, uint64_t blob_part_size ) override;

      auto tagList( const std::string& rsrc ) -> Tags override;
    protected:
    private:
      std::filesystem::directory_entry _directory;
  };
} // namespace OCI::Extensions
