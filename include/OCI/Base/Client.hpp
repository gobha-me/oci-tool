#pragma once
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
    virtual void fetchBlob( const std::string& rsrc, SHA256 sha, std::function< bool( const char *, uint64_t )>& call_back ) = 0;

    virtual auto hasBlob( const Schema1::ImageManifest& im, SHA256 sha ) -> bool = 0;
    virtual auto hasBlob( const Schema2::ImageManifest& im, const std::string& target, SHA256 sha ) -> bool = 0;

    virtual void putBlob( const Schema1::ImageManifest& im, const std::string& target, std::uintmax_t total_size, const char * blob_part, uint64_t blob_part_size ) = 0;
    virtual void putBlob( const Schema2::ImageManifest& im, const std::string& target, const SHA256& blob_sha, std::uintmax_t total_size, const char * blob_part, uint64_t blob_part_size ) = 0;

    virtual void fetchManifest( Schema1::ImageManifest& im, const std::string& rsrc, const std::string& target ) = 0;
    virtual void fetchManifest( Schema1::SignedImageManifest& sim, const std::string& rsrc, const std::string& target ) = 0;
    virtual void fetchManifest( Schema2::ManifestList& ml, const std::string& rsrc, const std::string& target ) = 0;
    virtual void fetchManifest( Schema2::ImageManifest& im, const std::string& rsrc, const std::string& target ) = 0;

    virtual void putManifest( const Schema1::ImageManifest& im, const std::string& rsrc, const std::string& target ) = 0;
    virtual void putManifest( const Schema1::SignedImageManifest& sim, const std::string& rsrc, const std::string& target ) = 0;
    virtual void putManifest( const Schema2::ManifestList& ml, const std::string& rsrc, const std::string& target ) = 0;
    virtual void putManifest( const Schema2::ImageManifest& im, const std::string& rsrc, const std::string& target ) = 0;

    virtual auto tagList( const std::string& rsrc ) -> Tags = 0;
  protected:
  private:
  };
} // namespace OCI::Base
