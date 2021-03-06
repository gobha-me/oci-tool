#include <OCI/Extensions/Dir.hpp>
#include <digestpp/algorithm/sha2.hpp>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <random>
#include <set>
#include <spdlog/spdlog.h>

std::mutex DIR_MUTEX;
std::mutex DIR_MAP_MUT;
std::mutex SEED_MUTEX;

auto genUUID() -> std::string {
  // clang-format off
  constexpr std::array< char, 62 > const CHARS( {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
    'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
    'u', 'v', 'w', 'x', 'y', 'z',
  } );
  // clang-format on
  constexpr auto WORD_SIZE = 48;

  std::string                     retVal;
  std::random_device              rd;
  std::mt19937                    generator( rd() );
  std::uniform_int_distribution<> dis( 0, CHARS.size() - 1 );

  for ( std::uint16_t index = 0; index != WORD_SIZE; index++ ) {
    retVal += CHARS.at( dis( generator ) );
  }

  return retVal;
}

auto validateFile( std::string const &sha, std::filesystem::path const &file ) -> bool {
  bool retVal = false;

  if ( std::filesystem::exists( file ) ) {
    std::ifstream blob( file, std::ios::binary );

    std::string sha256_str = digestpp::sha256().absorb( blob ).hexdigest();

    if ( sha == "sha256:" + sha256_str ) {
      retVal = true;
    }
  }

  return retVal;
}

OCI::Extensions::Dir::Dir() : bytes_written_( 0 ) {}
OCI::Extensions::Dir::Dir( std::string const &directory ) : bytes_written_( 0 ) {
  // tree_root_
  //   - first run empty
  //   - post has blobs and temp dirs w/ 1 - n domains
  // directory_
  //   - as destination will equal tree_root_
  //   - as source is tree_root_ + domain to copy
  auto trailing_slash = directory.find_last_of( '/' );

  if ( trailing_slash == directory.size() - 1 ) {
    directory_ = std::filesystem::directory_entry( directory.substr( 0, trailing_slash ) );
  } else {
    directory_ = std::filesystem::directory_entry( directory );
  }

  if ( not directory_.is_directory() ) {
    spdlog::error( "{} does not exist or is not a directory.", directory_.path().c_str() );
    std::abort();
  }

  tree_root_ = directory_;
  blobs_dir_ = std::filesystem::directory_entry( directory_.path() / "blobs" );
  temp_dir_  = std::filesystem::directory_entry( directory_.path() / "temp" );

  if ( std::filesystem::is_empty( directory_ ) ) {
    std::filesystem::create_directories( blobs_dir_ );
    std::filesystem::create_directories( temp_dir_ );
  }

  if ( not( blobs_dir_.is_directory() and temp_dir_.is_directory() ) ) {
    tree_root_ = std::filesystem::directory_entry( directory_.path().parent_path() );
    blobs_dir_ = std::filesystem::directory_entry( tree_root_.path() / "blobs" );
    temp_dir_  = std::filesystem::directory_entry( tree_root_.path() / "temp" );

    if ( not( blobs_dir_.is_directory() and temp_dir_.is_directory() ) ) {
      spdlog::error( "OCI::Extensions::Dir {} could not be determined to be a valid OCITree", directory );
      std::abort();
    }
  }
}

OCI::Extensions::Dir::Dir( OCI::Extensions::Dir const &other ) : bytes_written_( 0 ) {
  tree_root_ = other.tree_root_;
  directory_ = other.directory_;
  blobs_dir_ = other.blobs_dir_;
  temp_dir_  = other.temp_dir_;
}

OCI::Extensions::Dir::Dir( OCI::Extensions::Dir &&other ) noexcept {
  bytes_written_ = other.bytes_written_;
  tree_root_     = std::move( other.tree_root_ );
  directory_     = std::move( other.directory_ );
  blobs_dir_     = std::move( other.blobs_dir_ );
  temp_dir_      = std::move( other.temp_dir_ );
}

OCI::Extensions::Dir::~Dir() {
  if ( not temp_file_.empty() and std::filesystem::exists( temp_file_ ) ) {
    spdlog::error( "OCI::Extensions::Dir::~Dir cleaning up left over file {}", temp_file_.string() );
    std::filesystem::remove( temp_file_ );
  }
}

auto OCI::Extensions::Dir::operator=( Dir const &other ) -> Dir & {
  Dir temp( other );
  *this = std::move( temp );

  return *this;
}

auto OCI::Extensions::Dir::operator=( Dir &&other ) noexcept -> Dir & {
  tree_root_ = std::move( other.tree_root_ );
  directory_ = std::move( other.directory_ );
  blobs_dir_ = std::move( other.blobs_dir_ );
  temp_dir_  = std::move( other.temp_dir_ );

  return *this;
}

auto OCI::Extensions::Dir::copy() -> std::unique_ptr< OCI::Base::Client > {
  auto uc = std::make_unique< OCI::Extensions::Dir >( *this );

  uc->temp_file_.clear();

  return uc;
}

auto OCI::Extensions::Dir::catalog() -> const OCI::Catalog & {
  static Catalog retVal;
  auto const &   dir_map = dirMap(); // Can I efficiently replace this with a directory query

  if ( retVal.repositories.empty() ) {
    // retVal.repositories.resize( dir_map.size() ); // map size is twice the number of actual repos??? what

    std::transform( dir_map.begin(), dir_map.end(), back_inserter( retVal.repositories ),
                    []( auto const &pair ) { return pair.first; } );
  }

  return retVal;
}

auto OCI::Extensions::Dir::fetchBlob( [[maybe_unused]] const std::string &rsrc, SHA256 sha,
                                      std::function< bool( const char *, uint64_t ) > &call_back ) -> bool {
  bool retVal    = true;
  auto blob_path = tree_root_.path() / "blobs" / sha;

  spdlog::info( "OCI::Extensions::Dir::fetchBlob Fetching Blob Resource: {}", sha );
  if ( std::filesystem::exists( blob_path ) ) {
    constexpr auto BUFFSIZE = 4096; // 16384

    std::ifstream       blob( blob_path, std::ios::binary );
    std::vector< char > buf{ 0 };
    buf.reserve( BUFFSIZE );

    while ( blob.good() and retVal ) {
      blob.read( buf.data(), buf.capacity() );
      if ( blob.gcount() == 0 ) {
        throw std::runtime_error( "OCI::Extensions::Dir::fetchBlob read failed on " + blob_path.string() );
      } else {
        retVal = call_back( buf.data(), blob.gcount() );
      }
    }
  } else {
    spdlog::error( "OCI::Extensions::Dir::fetchBlob unable to locate blob: {}", sha );
  }

  return retVal;
}

auto OCI::Extensions::Dir::hasBlob( const Schema1::ImageManifest &im, [[maybe_unused]] SHA256 sha ) -> bool {
  bool                             retVal = false;
  std::filesystem::directory_entry image_dir_path;

  if ( directory_ == tree_root_ ) {
    image_dir_path = std::filesystem::directory_entry( tree_root_.path() / im.originDomain /
                                                       ( im.name + ":" + im.requestedTarget ) );
  } else {
    image_dir_path = std::filesystem::directory_entry( directory_.path() / ( im.name + ":" + im.requestedTarget ) );
  }

  if ( std::filesystem::exists( image_dir_path ) ) {
    retVal = true;
  }

  return retVal;
}

auto OCI::Extensions::Dir::hasBlob( Schema2::ImageManifest const &im, std::string const &target, SHA256 sha ) -> bool {
  bool                             retVal = false;
  std::filesystem::directory_entry image_dir_path;

  if ( directory_ == tree_root_ ) {
    image_dir_path = std::filesystem::directory_entry( tree_root_.path() / im.originDomain /
                                                       ( im.name + ":" + im.requestedTarget ) / target );
  } else {
    image_dir_path =
        std::filesystem::directory_entry( directory_.path() / ( im.name + ":" + im.requestedTarget ) / target );
  }

  // auto image_path = image_dir_path.path() / sha;
  auto image_path = blobs_dir_.path() / sha;

  if ( std::filesystem::exists( image_path ) ) {
    retVal = true;
  }
  //  else {
  //    auto blob_path = blobs_dir_.path() / sha;
  //
  //    retVal = createSymlink( blob_path, image_path );
  //
  //    if ( retVal ) {
  //      spdlog::info( "OCI::Extensions::Dir::hasBlob image {} already exists, created link to {}", sha,
  //      image_path.string() );
  //    }
  //  }

  return retVal;
}

auto OCI::Extensions::Dir::putBlob( const Schema1::ImageManifest &im, const std::string &target,
                                    std::uintmax_t total_size, const char *blob_part, uint64_t blob_part_size )
    -> bool {
  (void)im;
  (void)target;
  (void)total_size;
  (void)blob_part;
  (void)blob_part_size;

  auto retVal = false;

  spdlog::warn( "OCI::Extensions::Dir::putBlob Schema1::ImageManifest is not implemented" );

  return retVal;
}

auto OCI::Extensions::Dir::putBlob( Schema2::ImageManifest const &im, std::string const &target, SHA256 const &blob_sha,
                                    std::uintmax_t total_size, const char *blob_part, uint64_t blob_part_size )
    -> bool {
  auto retVal     = false;
  auto clean_file = false;
  if ( blobs_dir_.path().empty() ) {
    throw std::runtime_error( "OCI::Extentsions::Dir::pubBlob blobs_dir_ is empty" );
  }
  if ( directory_.path().empty() ) {
    throw std::runtime_error( "OCI::Extentsions::Dir::pubBlob directory_ is empty" );
  }
  auto blob_path      = blobs_dir_.path() / blob_sha;
  auto image_dir_path = std::filesystem::directory_entry( directory_.path() / im.originDomain /
                                                          ( im.name + ":" + im.requestedTarget ) / target );
  auto image_path     = image_dir_path.path() / blob_sha;

  if ( not image_dir_path.exists() ) {
    std::lock_guard< std::mutex > lg( DIR_MUTEX );

    if ( not image_dir_path.exists() ) {
      std::filesystem::create_directories( image_dir_path );
    }
  }

  if ( std::filesystem::exists( blob_path ) ) {
    clean_file = true;
    retVal     = true;
  } else {
    if ( temp_file_.empty() ) {
      std::lock_guard< std::mutex > lg( DIR_MUTEX );
      temp_file_ = temp_dir_.path() / genUUID();
      while ( std::filesystem::exists( temp_file_ ) ) {
        temp_file_ = temp_dir_.path() / genUUID();
      }
    }

    { // scoped so the file closes prior to any other operation
      std::ofstream blob( temp_file_, std::ios::app | std::ios::binary );

      if ( blob.good() ) {
        retVal = blob.write( blob_part, blob_part_size ).good();
      } else {
        spdlog::error( "OCI::Extensions::Dir::putBlob Failed to open {}", temp_file_.c_str() );
      }
    }

    if ( retVal ) {
      bytes_written_ += blob_part_size;
    } else {
      spdlog::error( "OCI::Extensions::Dir::putBlob failed to write data to {}", temp_file_.c_str() );
    }

    if ( bytes_written_ == total_size ) {
      if ( validateFile( blob_sha, temp_file_ ) ) {
        if ( not std::filesystem::exists( blob_path ) ) {
          std::lock_guard< std::mutex > lg( DIR_MUTEX );

          if ( not std::filesystem::exists( blob_path ) ) {
            std::filesystem::copy_file( temp_file_, blob_path );
          }
        }
      } else {
        spdlog::error( "OCI::Extensions::Dir Removed dirty file, file did not validate" );

        retVal = false;
      }

      clean_file = true;
    }
  }

  if ( clean_file ) {
    //    createSymlink( blob_path, image_path ); // returned boolean and not testing, was this a bug?

    if ( not temp_file_.empty() and std::filesystem::exists( temp_file_ ) ) {
      std::filesystem::remove( temp_file_ );
    }

    temp_file_.clear();
    bytes_written_ = 0;
  }

  return retVal;
}

void OCI::Extensions::Dir::fetchManifest( Schema1::ImageManifest &im, Schema1::ImageManifest const &request ) {
  (void)im;
  (void)request;

  spdlog::warn( "OCI::Extensions::Dir::fetchManifest Schema1::ImageManifest is not implemented" );
}

void OCI::Extensions::Dir::fetchManifest( Schema1::SignedImageManifest &      sim,
                                          Schema1::SignedImageManifest const &request ) {
  (void)sim;
  (void)request;

  spdlog::warn( "OCI::Extensions::Dir::fetchManifest Schema1::SignedImageManifest is not implemented" );
}

void OCI::Extensions::Dir::fetchManifest( Schema2::ManifestList &ml, Schema2::ManifestList const &request ) {
  // Expectation is we are in <base>/<domain> dir with a sub-tree of <repo>:<tags>
  std::filesystem::directory_entry ml_dir_path;
  std::filesystem::path            ml_file_path;
  std::filesystem::path            ver_file_path;

  if ( tree_root_ == directory_ ) {
    ml_dir_path = std::filesystem::directory_entry( tree_root_.path() / request.originDomain /
                                                    ( request.name + ":" + request.requestedTarget ) );
  } else {
    ml_dir_path =
        std::filesystem::directory_entry( directory_.path() / ( request.name + ":" + request.requestedTarget ) );
  }

  ml_file_path  = ml_dir_path.path() / "ManifestList.json";
  ver_file_path = ml_dir_path.path() / "Version";

  if ( std::filesystem::exists( ver_file_path ) ) {
    std::ifstream ver_file( ver_file_path );
    ver_file >> ml.schemaVersion;
  }

  if ( ml.schemaVersion == 2 and std::filesystem::exists( ml_file_path ) ) {
    nlohmann::json ml_json;

    std::ifstream ml_file( ml_file_path );

    bool file_parsed{ false };
    try {
      ml_file >> ml_json;
      file_parsed = true;
    } catch ( nlohmann::detail::parse_error &e ) {
      spdlog::critical( "Fatal error '{}', while working on {}:{}", e.what(), request.name, request.requestedTarget );
    } catch ( std::ios_base::failure &e ) {
      spdlog::critical( "Fatal error '{}', while working on {}", e.what(), ml_file_path.c_str() );
    }

    if ( file_parsed ) {
      ml_json.get_to( ml );

      ml_json[ "originDomain" ].get_to( ml.originDomain );
      ml_json[ "requestedTarget" ].get_to( ml.requestedTarget );
      ml_json[ "name" ].get_to( ml.name );
    }
  }
}

void OCI::Extensions::Dir::fetchManifest( Schema2::ImageManifest &im, Schema2::ImageManifest const &request ) {
  std::filesystem::directory_entry im_dir_path;
  std::filesystem::path            im_file_path;

  if ( tree_root_ == directory_ ) {
    im_dir_path =
        std::filesystem::directory_entry( tree_root_.path() / request.originDomain /
                                          ( request.name + ":" + request.requestedTarget ) / request.requestedDigest );
  } else {
    im_dir_path = std::filesystem::directory_entry(
        directory_.path() / ( request.name + ":" + request.requestedTarget ) / request.requestedDigest );
  }

  im_file_path = im_dir_path.path() / "ImageManifest.json";

  if ( std::filesystem::exists( im_file_path ) ) {
    nlohmann::json im_json;
    auto           read_file = false;

    do {
      if ( not read_file ) {
        try {
          std::ifstream im_file( im_file_path );
          im_file >> im_json;
          read_file = true;
        } catch ( nlohmann::detail::parse_error &e ) {
          spdlog::critical( "Fatal error '{}', while working on {}:{}/{}", e.what(), request.name,
                            request.requestedTarget, request.requestedDigest );
          break;
        } catch ( std::ios_base::failure &e ) {
          spdlog::critical( "Fatal error '{}', while working on {}", e.what(), im_file_path.c_str() );
          break;
        }
      } else {
        break;
      }
    } while ( not read_file );

    if ( read_file ) {
      im_json.get_to( im );

      im_json[ "originDomain" ].get_to( im.originDomain );
      im_json[ "requestedTarget" ].get_to( im.requestedTarget );
      im_json[ "requestedDigest" ].get_to( im.requestedDigest );
      im_json[ "name" ].get_to( im.name );
    } else {
      spdlog::error( "OCI::Extensions::Dir::fetchManifest Error reading ImageManifest.json {}:{}", request.name,
                     request.requestedTarget );
    }
  } else {
    spdlog::warn( "OCI::Extensions::Dir::fetchManifest Unable to locate ImageManifest for {}:{}", request.name,
                  request.requestedTarget );
  }
}

auto OCI::Extensions::Dir::putManifest( Schema1::ImageManifest const &im, std::string const &target ) -> bool {
  bool retVal = true;
  (void)im;
  (void)target;

  spdlog::warn( "OCI::Extensions::Dir::putManifest Schema1::ImageManifest is not implemented" );

  return retVal;
}

auto OCI::Extensions::Dir::putManifest( Schema1::SignedImageManifest const &sim, std::string const &target ) -> bool {
  bool retVal = true;
  (void)sim;
  (void)target;

  spdlog::warn( "OCI::Extensions::Dir::putManifest Schema1::SignedImageManifest is not implemented" );

  return retVal;
}

auto OCI::Extensions::Dir::putManifest( Schema2::ManifestList const &ml, [[maybe_unused]] std::string const &target )
    -> bool {
  bool retVal = false;
  auto manifest_list_dir_path =
      std::filesystem::directory_entry( tree_root_.path() / ml.originDomain / ( ml.name + ":" + ml.requestedTarget ) );
  auto manifest_list_path = manifest_list_dir_path.path() / "ManifestList.json";
  auto version_path       = manifest_list_dir_path.path() / "Version";

  if ( manifest_list_dir_path.exists() ) {
    for ( auto const &file : std::filesystem::directory_iterator( manifest_list_dir_path ) ) {
      if ( file.is_directory() ) {
        auto file_str = file.path().filename().string();
        auto mlm_itr  = std::find_if(
            ml.manifests.begin(), ml.manifests.end(),
            [ file_str ]( Schema2::ManifestList::Manifest const &mlm ) -> bool { return mlm.digest == file_str; } );

        if ( mlm_itr == ml.manifests.end() ) {
          std::lock_guard< std::mutex > lg( DIR_MUTEX );

          if ( file.exists() ) {
            spdlog::info( "OCI::Extensions::putManifest {} is not a ImageManifest of {}:{}", file.path().string(),
                          ml.name, ml.requestedTarget );
            std::filesystem::remove_all( file );
          }
        }
      }
    }
  }

  if ( std::filesystem::exists( manifest_list_path ) ) {
    Schema2::ManifestList other;
    fetchManifest( other, ml );

    if ( ml != other ) {
      std::lock_guard< std::mutex > lg( DIR_MUTEX );

      if ( std::filesystem::exists( manifest_list_path ) ) {
        spdlog::info( "OCI::Extensions::putManifest Schema2::ManifestList -> Received a new Manifest" );
        std::filesystem::remove( manifest_list_path );
      }
    }
  }

  nlohmann::json manifest_list_json = ml;

  // Output the Extensions for later reads
  manifest_list_json[ "originDomain" ]    = ml.originDomain;
  manifest_list_json[ "requestedTarget" ] = ml.requestedTarget;
  manifest_list_json[ "name" ]            = ml.name;

  auto valid = true;
  for ( auto const &im : ml.manifests ) {
    auto im_path = manifest_list_dir_path.path() / im.digest / "ImageManifest.json";

    if ( not std::filesystem::exists( im_path ) ) {
      spdlog::error( "OCI::Extensions::Dir::putManifest Unable to write ManifestList, missing ImageManifest: \n {}",
                     im_path.string() );
      valid = false;
    }
  }

  if ( valid and not std::filesystem::exists( manifest_list_path ) ) {
    spdlog::info( "OCI::Extensions::Dir::putManifest Schema2::ManifestList -> Writing file " );

    std::lock_guard< std::mutex > lg( DIR_MUTEX );
    std::ofstream                 manifest_list( manifest_list_path );
    std::ofstream                 version( version_path );

    manifest_list << std::setw( 2 ) << manifest_list_json;
    version << ml.schemaVersion;

    retVal = true;
  }

  return retVal;
} // OCI::Extensions::Dir::putManifest Schema2::ManifestList

auto OCI::Extensions::Dir::putManifest( Schema2::ImageManifest const &im, std::string const &target ) -> bool {
  bool retVal              = true;
  auto image_dir_path      = std::filesystem::directory_entry( tree_root_.path() / im.originDomain /
                                                          ( im.name + ":" + im.requestedTarget ) / target );
  auto image_manifest_path = image_dir_path.path() / "ImageManifest.json";

  nlohmann::json image_manifest_json = im;

  // Output the Extensions for later reads
  image_manifest_json[ "originDomain" ]    = im.originDomain;
  image_manifest_json[ "requestedTarget" ] = im.requestedTarget;
  image_manifest_json[ "requestedDigest" ] = im.requestedDigest;
  image_manifest_json[ "name" ]            = im.name;

  for ( auto const &layer : im.layers ) {
    if ( not hasBlob( im, target, layer.digest ) ) {
      spdlog::error( "OCI::Extensions::Dir::put Manifest Layer digest '{}' missing for {}:{}/{}", layer.digest, im.name,
                     im.requestedTarget, im.requestedDigest );
      retVal = false;
    }
  }

  if ( not hasBlob( im, target, im.config.digest ) ) {
    spdlog::error( "OCI::Extensions::Dir::put Manifest Config '{}' digest missing for {}:{}/{}", im.config.digest,
                   im.name, im.requestedTarget, im.requestedDigest );
    retVal = false;
  }

  if ( retVal ) {
    if ( std::filesystem::exists( image_manifest_path ) ) {
      retVal = false; // Because nothing changed
    } else {
      spdlog::info( "OCI::Extensions::put Manifest Schema::ImageManifest -> Writing file" );
      std::ofstream image_manifest( image_manifest_path );

      image_manifest << std::setw( 2 ) << image_manifest_json;
    }
  }

  return retVal;
}

auto OCI::Extensions::Dir::swap( Dir &other ) -> void {
  std::swap( bytes_written_, other.bytes_written_ );
  std::swap( tree_root_, other.tree_root_ );
  std::swap( directory_, other.directory_ );
  std::swap( blobs_dir_, other.blobs_dir_ );
  std::swap( temp_dir_, other.temp_dir_ );
}

auto OCI::Extensions::Dir::tagList( std::string const &rsrc ) -> OCI::Tags {
  OCI::Tags   retVal;
  auto const &dir_map = dirMap(); // Can I efficiently replace this with a directory query

  if ( dir_map.find( rsrc ) != dir_map.end() ) {
    retVal.name = rsrc;
    retVal.tags = dir_map.at( rsrc ).tags;
  }

  return retVal;
}

auto OCI::Extensions::Dir::tagList( std::string const &rsrc, std::regex const & /*re*/ ) -> OCI::Tags {
  auto retVal = tagList( rsrc );

  return retVal;
}

auto OCI::Extensions::Dir::dirMap() -> DirMap const & {
  static std::map< std::string, DirMap > retVal;
  auto                                   dir = directory_;

  // expecting dir to be root of the tree
  //  - subtree of namespaces
  //   - subtree of repo-name:(tag|digest) (contents varies based on Schema version)
  //    - subtree of digest with ImageManifests and blobs (Schemav2 only)
  // Top level can be either the root dir with domain subtrees or within a domain dir as tree root

  if ( retVal[ directory_.path().string() ].empty() ) {
    std::lock_guard< std::mutex > lg( DIR_MAP_MUT );

    if ( retVal[ directory_.path().string() ].empty() ) {
      spdlog::info( "OCI::Extensions::Dir::dirMap Generating Directory Map of: {}", directory_.path().string() );
      auto &dir_map  = retVal[ directory_.path().string() ];
      auto  base_dir = dir;

      for ( auto const &path_part : std::filesystem::recursive_directory_iterator( dir ) ) {
        if ( path_part.is_directory() and path_part.path().parent_path() == dir and
             path_part.path().filename().string().find( '.' ) !=
                 std::string::npos ) { // Will not work with shortnames, requires fqdn
          base_dir = path_part; // This is to account for the domain case, for a Docker -> Dir, where we may already
                                // have images
        } else if ( path_part.path().string().find( ':' ) != std::string::npos ) {
          auto repo_str =
              path_part.path().string().substr( base_dir.path().string().size() + 1 ); // + 1 to remove trailing slash

          if ( path_part.is_directory() ) {
            if ( std::count( repo_str.begin(), repo_str.end(), '/' ) == 1 ) {
              auto tag       = repo_str.substr( repo_str.find( ':' ) + 1 );
              auto repo_name = repo_str.substr( 0, repo_str.find( ':' ) );

              dir_map[ repo_name ].tags.push_back( tag );

              if ( std::filesystem::exists( path_part.path() / "ManifestList.json" ) ) {
                dir_map[ repo_name ].path[ tag ] = path_part;
              }
            } else {
              auto repo_name = repo_str.substr( 0, repo_str.find( ':' ) );
              auto target    = repo_str.substr( repo_str.find_last_of( '/' ) + 1 );

              dir_map[ repo_name ].path[ target ] = path_part;
            }
          } else if ( repo_str.find( "sha" ) != std::string::npos and repo_str.find( "json" ) == std::string::npos ) {
            // Blobs
            auto repo_name = repo_str.substr( 0, repo_str.find( ':' ) );
            auto target    = repo_str.substr( repo_str.find_last_of( '/' ) + 1 );

            dir_map[ repo_name ].path[ target ] = path_part;
          }
        }
      }
    }
  }

  return retVal[ directory_.path().string() ];
}

auto OCI::Extensions::Dir::createSymlink( std::filesystem::path &blob_path, std::filesystem::path &image_path )
    -> bool {
  auto retVal = false;

  if ( std::filesystem::exists( blob_path ) ) {
    if ( not std::filesystem::exists( image_path.parent_path() ) ) {
      std::lock_guard< std::mutex > lg( DIR_MUTEX );

      if ( not std::filesystem::exists( image_path.parent_path() ) ) {
        std::filesystem::create_directories( image_path.parent_path() );
        spdlog::info( "OCI::Extensions::Dir::createSymlink created dirictory {}", image_path.parent_path().string() );
      }
    }

    if ( not std::filesystem::exists( image_path ) ) {
      std::lock_guard< std::mutex > lg( DIR_MUTEX );

      if ( not std::filesystem::exists( image_path ) ) {
        std::error_code ec;

        std::filesystem::create_symlink( blob_path, image_path, ec );

        if ( ec and ec.value() != 17 ) { // NOLINT FILE EXISTS
          spdlog::error( "OCI::Extensions::Dir::createSymlink create_symlink( {} -> {} ) {} -> {}", ec.value(),
                         ec.message(), blob_path.string(), image_path.string() );
        } else {
          retVal = true;
        }
      }
    }
  }

  return retVal;
}
