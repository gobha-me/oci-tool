#pragma once
#include <OCI/Base/Client.hpp>
#include <OCI/Schema1.hpp>
#include <OCI/Schema2.hpp>
#include <OCI/Tags.hpp>
#include <filesystem>
#include <mutex>

namespace OCI::Extensions {
  using RepoName = std::string;
  using Target   = std::string;
  using Tags     = std::vector< std::string >;

  struct RepoDetails {
    Tags                                                 tags;
    std::map< Target, std::filesystem::directory_entry > path;
  };

  // FIXME: Make room for the domain?
  using DirMap = std::map< RepoName, RepoDetails >;

  class Dir : public OCI::Base::Client {
  public:
    Dir();
    explicit Dir( std::string const &directory ); // this would be the base path
    Dir( Dir const &other );
    Dir( Dir &&other ) noexcept;

    ~Dir() override;

    auto operator=( Dir const &other ) -> Dir &;
    auto operator=( Dir &&other ) noexcept -> Dir &;

    auto catalog() -> const OCI::Catalog & override;

    auto copy() -> std::unique_ptr< OCI::Base::Client > override;

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

    auto swap( Dir &other ) -> void;

    auto tagList( const std::string &rsrc ) -> OCI::Tags override;
    auto tagList( std::string const &rsrc, std::regex const &re ) -> OCI::Tags override;

  protected:
    auto        dirMap() -> DirMap const &;
    static auto createSymlink( std::filesystem::path &blob_path, std::filesystem::path &image_path ) -> bool;

  private:
    uint64_t                         bytes_written_;
    std::filesystem::directory_entry directory_;
    std::filesystem::directory_entry tree_root_;
    std::filesystem::directory_entry blobs_dir_;
    std::filesystem::directory_entry temp_dir_;
    std::filesystem::path            temp_file_;
  };
} // namespace OCI::Extensions
