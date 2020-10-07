#pragma once
#include <OCI/Catalog.hpp>
#include <OCI/Schema1.hpp>
#include <OCI/Schema2.hpp>
#include <OCI/Tags.hpp>
#include <functional>
#include <string>
#include <regex>

namespace OCI::Base {
  class Client {
  public:
    using SHA256 = std::string; // kinda preemptive, incase this becomes a real type

    Client() = default;
    Client( Client const& ) = default;
    Client( Client && ) = default;
    virtual ~Client() = default;

    auto operator=( Client const& ) -> Client& = default;
    auto operator=( Client && ) -> Client& = default;

    virtual auto catalog() -> const OCI::Catalog& = 0;

    virtual auto copy() -> std::unique_ptr< Client > = 0;

    virtual auto fetchBlob( std::string const& rsrc, SHA256 sha, std::function< bool( const char *, uint64_t )>& call_back ) -> bool = 0;

    virtual auto hasBlob( Schema1::ImageManifest const& im, SHA256 sha ) -> bool = 0;
    virtual auto hasBlob( Schema2::ImageManifest const& im, std::string const& target, SHA256 sha ) -> bool = 0;

    virtual auto putBlob( Schema1::ImageManifest const& im, std::string const& target, std::uintmax_t total_size, const char * blob_part,    uint64_t blob_part_size ) -> bool = 0;
    virtual auto putBlob( Schema2::ImageManifest const& im, std::string const& target, SHA256  const& blob_sha,   std::uintmax_t total_size, const char * blob_part, uint64_t blob_part_size ) -> bool = 0;
                                                                             
    virtual void fetchManifest( Schema1::ImageManifest      & im,  Schema1::ImageManifest       const& request ) = 0;
    virtual void fetchManifest( Schema1::SignedImageManifest& sim, Schema1::SignedImageManifest const& request ) = 0;
    virtual void fetchManifest( Schema2::ManifestList       & ml,  Schema2::ManifestList        const& request ) = 0;
    virtual void fetchManifest( Schema2::ImageManifest      & im,  Schema2::ImageManifest       const& request ) = 0;

    virtual auto putManifest( Schema1::ImageManifest       const& im,  std::string const& target ) -> bool = 0;
    virtual auto putManifest( Schema1::SignedImageManifest const& sim, std::string const& target ) -> bool = 0;
    virtual auto putManifest( Schema2::ManifestList        const& ml,  std::string const& target ) -> bool = 0;
    virtual auto putManifest( Schema2::ImageManifest       const& im,  std::string const& target ) -> bool = 0;

    virtual auto tagList( std::string const& rsrc ) -> Tags = 0;
    virtual auto tagList( std::string const& rsrc, std::regex const& re ) -> Tags = 0;
  protected:
  private:
  };
} // namespace OCI::Base
