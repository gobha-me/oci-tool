#pragma once
#include <OCI/Base/Client.hpp>
#include <OCI/Manifest.hpp>
#include <OCI/Registry/Error.hpp>
#include <OCI/Tags.hpp>
#include <future>
#include <iostream> // Will want to remove this at somepoint
#include <nlohmann/json.hpp> // https://github.com/nlohmann/json
#include <string>
#include <vector>

#define CPPHTTPLIB_OPENSSL_SUPPORT = 1 //NOLINT(cppcoreguidelines-macro-usage)
#include <httplib.h> // https://github.com/yhirose/cpp-httplib

namespace OCI::Registry { // https://docs.docker.com/registry/spec/api/
  class Client : public Base::Client {
  public:
    Client();
    explicit Client( std::string const& domain );
    Client( std::string const& domain, std::string username, std::string password );
    ~Client() override = default;

    void auth( std::string const& rsrc );

    void fetchBlob( std::string const& rsrc, SHA256 sha, std::function<void(const char *, uint64_t )>& call_back ) override; // To where

    auto hasBlob( const Schema1::ImageManifest& im, SHA256 sha ) -> bool override;
    auto hasBlob( const Schema2::ImageManifest& im, SHA256 sha ) -> bool override;

    void putBlob( const Schema1::ImageManifest& im, std::string const& target, std::uintmax_t total_size, const char * blob_part, uint64_t blob_part_size ) override;
    void putBlob( const Schema2::ImageManifest& im, std::string const& target, std::uintmax_t total_size, const char * blob_part, uint64_t blob_part_size ) override;

    void inspect( std::string const& rsrc, std::string const& target );

    void fetchManifest( Schema1::ImageManifest& im,        std::string const& rsrc, std::string const& target ) override;
    void fetchManifest( Schema1::SignedImageManifest& sim, std::string const& rsrc, std::string const& target ) override;
    void fetchManifest( Schema2::ManifestList& ml,         std::string const& rsrc, std::string const& target ) override;
    void fetchManifest( Schema2::ImageManifest& im,        std::string const& rsrc, std::string const& target ) override;

    void putManifest( const Schema1::ImageManifest& im, const std::string& rsrc, const std::string& target ) override;
    void putManifest( const Schema1::SignedImageManifest& sim, const std::string& rsrc, const std::string& target ) override;
    void putManifest( const Schema2::ManifestList& ml, const std::string& rsrc, const std::string& target ) override;
    void putManifest( const Schema2::ImageManifest& im, const std::string& rsrc, const std::string& target ) override;

    auto tagList( std::string const& rsrc ) -> Tags override;

    auto pingResource( std::string const& rsrc ) -> bool;
  protected:
    auto defaultHeaders() -> httplib::Headers; 
    auto fetchManifest( const std::string &mediaType, const std::string& resource, const std::string& target ) -> std::shared_ptr< httplib::Response >;
  private:
    std::shared_ptr< httplib::SSLClient >  _cli;
    std::string _domain;
    std::string _token;
    std::string _username;
    std::string _password;
  };
} // namespace OCI::Registry
