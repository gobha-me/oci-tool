#pragma once
#include <OCI/Base/Client.hpp>
#include <OCI/Schema1.hpp>
#include <OCI/Schema2.hpp>
#include <OCI/Tags.hpp>
#include <Yaml.hpp>
#include <filesystem>
#include <mutex>

namespace OCI::Extensions {
  class Yaml : public OCI::Base::Client {
  public:
    struct Catalog {
      using Domain   = std::string;
      using Username = std::string;
      using Password = std::string;
      using RepoName = std::string;

      std::map< Domain, OCI::Catalog >                    catalogs;
      std::map< Domain, std::pair< Username, Password > > credentials;
      std::vector< Domain >                               domains;
      std::vector< std::string >                          architectures;
      std::string                                         tag_filter;
      std::map< Domain, std::map< RepoName, OCI::Tags > > tags;
    };

    Yaml() = default;
    explicit Yaml( std::string const &file_path ); // this would be the base path
    Yaml( Yaml const &other );
    Yaml( Yaml &&other ) noexcept;

    ~Yaml() override = default;

    auto operator=( Yaml const &other ) -> Yaml &;
    auto operator=( Yaml &&other ) noexcept -> Yaml &;

    auto catalog() -> const OCI::Catalog& override;
    auto catalog( std::string const &domain ) -> OCI::Catalog;

    auto copy() -> std::unique_ptr< OCI::Base::Client > override;

    [[nodiscard]] auto domains() const -> std::vector< std::string > const &;

    auto fetchBlob( std::string const &rsrc, SHA256 sha, std::function< bool( const char *, uint64_t ) > &call_back )
        -> bool override;

    auto hasBlob( Schema1::ImageManifest const &im, SHA256 sha ) -> bool override;
    auto hasBlob( Schema2::ImageManifest const &im, std::string const &target, SHA256 sha ) -> bool override;

    auto putBlob( Schema1::ImageManifest const &im, std::string const &target, std::uintmax_t total_size,
                  const char *blob_part, uint64_t blob_part_size ) -> bool override;
    auto putBlob( Schema2::ImageManifest const &im, std::string const &target, SHA256 const &blob_sha,
                  std::uintmax_t total_size, const char *blob_part, uint64_t blob_part_size ) -> bool override;

    void fetchManifest( Schema1::ImageManifest &im, Schema1::ImageManifest const &request ) override;
    void fetchManifest( Schema1::SignedImageManifest &sim, Schema1::SignedImageManifest const &request ) override;
    void fetchManifest( Schema2::ManifestList &ml, Schema2::ManifestList const &request ) override;
    void fetchManifest( Schema2::ImageManifest &im, Schema2::ImageManifest const &request ) override;

    auto putManifest( Schema1::ImageManifest const &im, std::string const &target ) -> bool override;
    auto putManifest( Schema1::SignedImageManifest const &sim, std::string const &target ) -> bool override;
    auto putManifest( Schema2::ManifestList const &ml, std::string const &target ) -> bool override;
    auto putManifest( Schema2::ImageManifest const &im, std::string const &target ) -> bool override;

    auto tagList( std::string const &rsrc ) -> OCI::Tags override;
    auto tagList( std::string const &rsrc, std::regex const &re ) -> OCI::Tags override;

    auto swap( Yaml &other ) -> void;
  protected:
  private:
    mutable std::mutex                   _mutex;
    std::unique_ptr< OCI::Base::Client > _client;
    std::string                          _current_domain;
    Catalog                              _catalog;
  };
} // namespace OCI::Extensions
