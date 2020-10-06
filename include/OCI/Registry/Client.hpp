#pragma once
#include <OCI/Base/Client.hpp>
#include <OCI/Manifest.hpp>
#include <OCI/Registry/Error.hpp>
#include <OCI/Tags.hpp>
#include <chrono>
#include <future>
#include <iostream>          // Will want to remove this at somepoint
#include <nlohmann/json.hpp> // https://github.com/nlohmann/json
#include <regex>
#include <string>
#include <vector>

#define CPPHTTPLIB_OPENSSL_SUPPORT = 1 // NOLINT(cppcoreguidelines-macro-usage) - required for httplib to enable TLS
#include <httplib.h>                   // https://github.com/yhirose/cpp-httplib

namespace OCI::Registry { // https://docs.docker.com/registry/spec/api/
  class Client : public Base::Client {
  public:
    Client();
    explicit Client( std::string const &location );
    Client( std::string const &location, std::string username, std::string password );

    Client( Client const &other );
    Client( Client &&other ) noexcept;

    ~Client() override = default;

    auto operator=( Client const &other ) -> Client &;
    auto operator=( Client &&other ) noexcept -> Client &;

    void auth( httplib::Headers const &headers, std::string const &scope );

    auto catalog() -> OCI::Catalog override;

    auto copy() -> std::unique_ptr< OCI::Base::Client > override;

    auto fetchBlob( std::string const &rsrc, SHA256 sha, std::function< bool( const char *, uint64_t ) > &call_back )
        -> bool override;

    auto hasBlob( const Schema1::ImageManifest &im, SHA256 sha ) -> bool override;
    auto hasBlob( const Schema2::ImageManifest &im, const std::string &target, SHA256 sha ) -> bool override;

    auto putBlob( const Schema1::ImageManifest &im, std::string const &target, std::uintmax_t total_size,
                  const char *blob_part, uint64_t blob_part_size ) -> bool override;
    auto putBlob( const Schema2::ImageManifest &im, std::string const &target, const SHA256 &blob_sha,
                  std::uintmax_t total_size, const char *blob_part, uint64_t blob_part_size ) -> bool override;

    void inspect( std::string const &rsrc, std::string const &target );

    void fetchManifest( Schema1::ImageManifest &im, Schema1::ImageManifest const &request ) override;
    void fetchManifest( Schema1::SignedImageManifest &sim, Schema1::SignedImageManifest const &request ) override;
    void fetchManifest( Schema2::ManifestList &ml, Schema2::ManifestList const &request ) override;
    void fetchManifest( Schema2::ImageManifest &im, Schema2::ImageManifest const &request ) override;

    auto putManifest( Schema1::ImageManifest const &im, std::string const &target ) -> bool override;
    auto putManifest( Schema1::SignedImageManifest const &sim, std::string const &target ) -> bool override;
    auto putManifest( Schema2::ManifestList const &ml, std::string const &target ) -> bool override;
    auto putManifest( Schema2::ImageManifest const &im, std::string const &target ) -> bool override;

    auto tagList( std::string const &rsrc ) -> Tags override;
    auto tagList( std::string const &rsrc, std::regex const& re ) -> Tags override;

    auto ping() -> bool;
    auto pingResource( std::string const &rsrc ) -> bool;

    auto swap( Client &other ) -> void;

    struct TokenResponse {
      std::string token;
      // std::string access_token; part of the proto, but supposed to equal token, undefined behavior otherwise
      std::chrono::seconds                  expires_in;
      std::chrono::system_clock::time_point issued_at;
      std::string                           refresh_token;
    };

  protected:
    [[nodiscard]] auto authHeaders() const -> httplib::Headers;
    auto               authExpired() -> bool;
    auto fetchManifest( const std::string &mediaType, const std::string &resource, const std::string &target )
        -> std::string const;

  private:
    bool                               _secure_con{false};
    std::shared_ptr< httplib::Client > _cli{ nullptr };
    std::unique_ptr< httplib::Client > _patch_cli{ nullptr };
    std::string                        _domain; // Required for making copies
    std::string                        _username;
    std::string                        _password;
    //    std::string _resource; // <namespace>/<repo name>
    //    std::string _requested_target; // tag or digest
    std::string _patch_location;

    TokenResponse _ctr;

    bool _auth_retry{ true };
  };

  void from_json( nlohmann::json const &j, Client::TokenResponse &ctr );
} // namespace OCI::Registry
