#pragma once
#include <OCI/Catalog.hpp>
#include <OCI/Schema1.hpp>
#include <OCI/Schema2.hpp>
#include <OCI/Tags.hpp>
#include <functional>
#include <string>

namespace OCI::Base {
  class Client {
  public:
    using SHA256 = std::string; // kinda preemptive, incase this becomes a real type
  public:
    virtual ~Client() = default;

    virtual auto catalog() -> OCI::Catalog = 0;

    virtual void fetchBlob( std::string const& rsrc, SHA256 sha, std::function< bool( const char *, uint64_t )>& call_back ) = 0;

    virtual auto hasBlob( Schema1::ImageManifest const& im, SHA256 sha ) -> bool = 0;
    virtual auto hasBlob( Schema2::ImageManifest const& im, std::string const& target, SHA256 sha ) -> bool = 0;

    virtual void putBlob( Schema1::ImageManifest const& im, std::string const& target, std::uintmax_t total_size, const char * blob_part, uint64_t blob_part_size ) = 0;
    virtual void putBlob( Schema2::ImageManifest const& im, std::string const& target, SHA256 const& blob_sha, std::uintmax_t total_size, const char * blob_part, uint64_t blob_part_size ) = 0;
                                                                             
    virtual void fetchManifest( Schema1::ImageManifest& im,        std::string const& rsrc, std::string const& target ) = 0;
    virtual void fetchManifest( Schema1::SignedImageManifest& sim, std::string const& rsrc, std::string const& target ) = 0;
    virtual void fetchManifest( Schema2::ManifestList& ml,         std::string const& rsrc, std::string const& target ) = 0;
    virtual void fetchManifest( Schema2::ImageManifest& im,        std::string const& rsrc, std::string const& target ) = 0;

    virtual void putManifest( Schema1::ImageManifest const& im,        std::string const& target ) = 0;
    virtual void putManifest( Schema1::SignedImageManifest const& sim, std::string const& target ) = 0;
    virtual void putManifest( Schema2::ManifestList const& ml,         std::string const& target ) = 0;
    virtual void putManifest( Schema2::ImageManifest const& im,        std::string const& target ) = 0;

    virtual auto tagList( std::string const& rsrc ) -> Tags = 0;
  protected:
  private:
  };
} // namespace OCI::Base
