#pragma once
#include <string>
#include <functional>
#include <OCI/Schema1.hpp>
#include <OCI/Schema2.hpp>
#include <OCI/Tags.hpp>

namespace OCI::Base {
  class Client {
  public:
    using SHA256 = std::string; // kinda preemptive, incase this becomes a real type
  public:
    virtual ~Client() = default;
    virtual void fetchBlob( const std::string& rsrc, SHA256 sha, std::function< void( const char *, uint64_t )>& call_back ) = 0;

    virtual bool hasBlob( const std::string& rsrc, SHA256 sha ) = 0;

    virtual void manifest( Schema1::ImageManifest& im, const std::string& rsrc, const std::string& target ) = 0;
    virtual void manifest( Schema1::SignedImageManifest& sim, const std::string& rsrc, const std::string& target ) = 0;
    virtual void manifest( Schema2::ManifestList& ml, const std::string& rsrc, const std::string& target ) = 0;
    virtual void manifest( Schema2::ImageManifest& im, const std::string& rsrc, const std::string& target ) = 0;

    virtual void putBlob( const std::string& rsrc, const std::string& target, std::uintmax_t total_size, const char * blob_part, uint64_t blob_part_size ) = 0;

    virtual Tags tagList( const std::string& rsrc ) = 0;
  protected:
  private:
  };
} // namespace OCI::Base
