#include <OCI/Extensions/Dir.hpp>
#include <botan/hash.h>
#include <botan/hex.h>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <random>
#include <set>
#include <spdlog/spdlog.h>

std::mutex DIR_MUTEX;
std::mutex DIR_MAP_MUT;
std::mutex SEED_MUTEX;

// clang-format off
auto genUUID() -> std::string {
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

  std::string retVal;
  std::random_device rd;
  std::mt19937 generator( rd() );
  std::uniform_int_distribution<> dis( 0, CHARS.size() - 1 );

  for ( std::uint16_t index = 0; index != WORD_SIZE; index++ ) {
    retVal += CHARS[ dis( generator ) ]; // NOLINT
  }

  return retVal;
}

auto validateFile( std::string const &sha, std::filesystem::path const &file ) -> bool {
  bool retVal = false;
  constexpr auto BUFFSIZE = 4096;

  if ( std::filesystem::exists( file ) ) {
    auto sha256( Botan::HashFunction::create( "SHA-256" ) );

    std::ifstream blob( file, std::ios::binary );
    std::vector< uint8_t > buf( BUFFSIZE );

    while ( blob.good() ) {
      blob.read( reinterpret_cast< char * >( buf.data() ), buf.size() ); // NOLINT
      size_t readcount = blob.gcount();
      sha256->update( buf.data(), readcount );
    }

    std::string sha256_str = Botan::hex_encode( sha256->final(), false );

    if ( sha == "sha256:" + sha256_str ) {
      retVal = true;
    }
  }

  return retVal;
}

OCI::Extensions::Dir::Dir() : _bytes_written( 0 ) {}
OCI::Extensions::Dir::Dir( std::string const &directory ) : _bytes_written( 0 ) {
  // _tree_root
  //   - first run empty
  //   - post has blobs and temp dirs w/ 1 - n domains
  // _directory
  //   - as destination will equal _tree_root
  //   - as source is _tree_root + domain to copy
  auto trailing_slash = directory.find_last_of( '/' );

  if ( trailing_slash == directory.size() - 1 ) {
    _directory = std::filesystem::directory_entry( directory.substr( 0, trailing_slash ) );
  } else {
    _directory = std::filesystem::directory_entry( directory );
  }

  if ( not _directory.is_directory() ) {
    std::cerr << _directory.path() << " does not exist or is not a directory.\n";
    std::abort();
  }

  _tree_root = _directory;
  _blobs_dir = std::filesystem::directory_entry( _directory.path() / "blobs" );
  _temp_dir = std::filesystem::directory_entry( _directory.path() / "temp" );

  if ( std::filesystem::is_empty( _directory ) ) {
    std::filesystem::create_directories( _blobs_dir );
    std::filesystem::create_directories( _temp_dir );
  }

  if ( not( _blobs_dir.is_directory() and _temp_dir.is_directory() ) ) {
    _tree_root = std::filesystem::directory_entry( _directory.path().parent_path() );
    _blobs_dir = std::filesystem::directory_entry( _tree_root.path() / "blobs" );
    _temp_dir = std::filesystem::directory_entry( _tree_root.path() / "temp" );

    if ( not( _blobs_dir.is_directory() and _temp_dir.is_directory() ) ) {
      spdlog::error( "OCI::Extensions::Dir {} could not be determined to be a valid OCITree", directory );
      std::abort();
    }
  }
}

OCI::Extensions::Dir::Dir( OCI::Extensions::Dir const &other ) : _bytes_written( 0 ) {
  _tree_root = other._tree_root;
  _directory = other._directory;
  _blobs_dir = other._blobs_dir;
  _temp_dir = other._temp_dir;
}

OCI::Extensions::Dir::Dir( OCI::Extensions::Dir &&other ) noexcept {
  _bytes_written = other._bytes_written;
  _tree_root = std::move( other._tree_root );
  _directory = std::move( other._directory );
  _blobs_dir = std::move( other._blobs_dir );
  _temp_dir = std::move( other._temp_dir );
}

auto OCI::Extensions::Dir::operator=( Dir const &other ) -> Dir & {
  Dir( other ).swap( *this );

  return *this;
}

auto OCI::Extensions::Dir::operator=( Dir &&other ) noexcept -> Dir & {
  _tree_root = std::move( other._tree_root );
  _directory = std::move( other._directory );
  _blobs_dir = std::move( other._blobs_dir );
  _temp_dir = std::move( other._temp_dir );

  return *this;
}

auto OCI::Extensions::Dir::copy() -> std::unique_ptr< OCI::Base::Client > {
  return std::make_unique< OCI::Extensions::Dir >( *this );
}

auto OCI::Extensions::Dir::catalog() -> OCI::Catalog {
  Catalog retVal;
  auto const &dir_map = dirMap(); // Can I efficiently replace this with a directory query

  retVal.repositories.resize( dir_map.size() );

  std::transform( dir_map.begin(), dir_map.end(), back_inserter( retVal.repositories ),
                  []( auto const &pair ) { return pair.first; } );

  return retVal;
}

auto OCI::Extensions::Dir::fetchBlob( [[maybe_unused]] const std::string &rsrc, SHA256 sha,
                                      std::function< bool( const char *, uint64_t ) > &call_back ) -> bool {
  bool retVal = true;
  auto blob_path = _tree_root.path() / "blobs" / sha;

  spdlog::info( "OCI::Extensions::Dir::fetchBlob Fetching Blob Resource: {}", sha );
  if ( std::filesystem::exists( blob_path ) ) {
    constexpr auto BUFFSIZE = 4096;

    std::ifstream blob( blob_path, std::ios::binary );
    std::vector< uint8_t > buf( BUFFSIZE );

    while ( blob.good() and retVal ) {
      blob.read( reinterpret_cast< char * >( buf.data() ), buf.size() ); // NOLINT
      size_t readcount = blob.gcount();
      retVal = call_back( reinterpret_cast< const char * >( buf.data() ), readcount ); // NOLINT
    }
  } else {
    spdlog::error( "OCI::Extensions::Dir::fetchBlob unable to locate blob: {}", sha );
  }

  return retVal;
}

auto OCI::Extensions::Dir::hasBlob( const Schema1::ImageManifest &im, [[maybe_unused]] SHA256 sha ) -> bool {
  bool retVal = false;
  std::filesystem::directory_entry image_dir_path;

  if ( _directory == _tree_root ) {
    image_dir_path = std::filesystem::directory_entry( _tree_root.path() / im.originDomain /
                                                       ( im.name + ":" + im.requestedTarget ) );
  } else {
    image_dir_path = std::filesystem::directory_entry( _directory.path() / ( im.name + ":" + im.requestedTarget ) );
  }

  if ( std::filesystem::exists( image_dir_path ) ) {
    retVal = true;
  }

  return retVal;
}

auto OCI::Extensions::Dir::hasBlob( Schema2::ImageManifest const &im, std::string const &target, SHA256 sha ) -> bool {
  bool retVal = false;
  std::filesystem::directory_entry image_dir_path;

  if ( _directory == _tree_root ) {
    image_dir_path = std::filesystem::directory_entry( _tree_root.path() / im.originDomain /
                                                       ( im.name + ":" + im.requestedTarget ) / target );
  } else {
    image_dir_path =
        std::filesystem::directory_entry( _directory.path() / ( im.name + ":" + im.requestedTarget ) / target );
  }

  auto image_path = image_dir_path.path() / sha;

  if ( std::filesystem::exists( image_path ) ) {
    retVal = true;
  }

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

  spdlog::error( "OCI::Extensions::Dir::putBlob Schema1::ImageManifest is not implemented" );

  return false;
}

auto OCI::Extensions::Dir::putBlob( Schema2::ImageManifest const &im, std::string const &target, SHA256 const &blob_sha,
                                    std::uintmax_t total_size, const char *blob_part, uint64_t blob_part_size )
    -> bool {
  auto retVal = false;
  auto complete = false;
  auto blob_path = _blobs_dir.path() / blob_sha;
  auto image_dir_path = std::filesystem::directory_entry( _directory.path() / im.originDomain /
                                                          ( im.name + ":" + im.requestedTarget ) / target );
  auto image_path = image_dir_path.path() / blob_sha;

  if ( not image_dir_path.exists() ) {
    std::lock_guard< std::mutex > lg( DIR_MUTEX );

    if ( not image_dir_path.exists() ) {
      std::filesystem::create_directories( image_dir_path );
    }
  }

  if ( std::filesystem::exists( blob_path ) ) {
    complete = true;
  } else {
    if ( _temp_file.empty() ) {
      _temp_file = _temp_dir.path() / genUUID();
    }

    { // scoped so the file closes prior to any other operation
      std::ofstream blob( _temp_file, std::ios::app | std::ios::binary );

      retVal = blob.write( blob_part, blob_part_size ).good();
    }

    if ( retVal ) {
      _bytes_written += blob_part_size;
    }

    if ( _bytes_written == total_size ) {
      if ( validateFile( blob_sha, _temp_file ) ) {
        if ( not std::filesystem::exists( blob_path ) ) {
          std::lock_guard< std::mutex > lg( DIR_MUTEX );

          if ( not std::filesystem::exists( blob_path ) ) {
            std::filesystem::copy_file( _temp_file, blob_path );
          }

          complete = true;
        }
      } else {
        spdlog::error( "OCI::Extensions::Dir Removed dirty file, file did not validate" );

        std::filesystem::remove( _temp_file );
        _temp_file.clear();
        _bytes_written = 0;

        retVal = false;
      }
    }
  }

  if ( complete ) {
    if ( not std::filesystem::exists( image_path ) ) {
      std::lock_guard< std::mutex > lg( DIR_MUTEX );

      if ( not std::filesystem::exists( image_path ) ) {
        std::error_code ec;

        std::filesystem::create_symlink( blob_path, image_path, ec );

        if ( ec and ec.value() != 17 ) { // NOLINT FILE EXISTS
          spdlog::error( "OCI::Extensions::putBlob create_symlink( {} -> {} ) {} -> {}", ec.value(), ec.message(),
                         blob_path.string(), image_path.string() );
        }
      }
    }

    if ( not _temp_file.empty() and std::filesystem::exists( _temp_file ) ) {
      std::filesystem::remove( _temp_file );
    }

    _temp_file.clear();
    _bytes_written = 0;
  }

  return retVal;
}

void OCI::Extensions::Dir::fetchManifest( Schema1::ImageManifest &im, Schema1::ImageManifest const &request ) {
  (void)im;
  (void)request;

  spdlog::warn( "OCI::Extensions::Dir::fetchManifest Schema1::ImageManifest is not implemented" );
}

void OCI::Extensions::Dir::fetchManifest( Schema1::SignedImageManifest &sim,
                                          Schema1::SignedImageManifest const &request ) {
  (void)sim;
  (void)request;

  spdlog::warn( "OCI::Extensions::Dir::fetchManifest Schema1::SignedImageManifest is not implemented" );
}

void OCI::Extensions::Dir::fetchManifest( Schema2::ManifestList &ml, Schema2::ManifestList const &request ) {
  // Expectation is we are in <base>/<domain> dir with a sub-tree of <repo>:<tags>
  std::filesystem::directory_entry ml_dir_path;
  std::filesystem::path ml_file_path;
  std::filesystem::path ver_file_path;

  if ( _tree_root == _directory ) {
    ml_dir_path = std::filesystem::directory_entry( _tree_root.path() / request.originDomain /
                                                    ( request.name + ":" + request.requestedTarget ) );
  } else {
    ml_dir_path =
        std::filesystem::directory_entry( _directory.path() / ( request.name + ":" + request.requestedTarget ) );
  }

  ml_file_path = ml_dir_path.path() / "ManifestList.json";
  ver_file_path = ml_dir_path.path() / "Version";

  if ( std::filesystem::exists( ver_file_path ) ) {
    std::ifstream ver_file( ver_file_path );
    ver_file >> ml.schemaVersion;
  }

  if ( ml.schemaVersion == 2 and std::filesystem::exists( ml_file_path ) ) {
    nlohmann::json ml_json;

    std::ifstream ml_file( ml_file_path );

    try {
      ml_file >> ml_json;
    } catch ( nlohmann::detail::parse_error &e ) {
      spdlog::critical( "Fatal error '{}', while working on {}:{}", e.what(), request.name, request.requestedTarget );
      std::terminate();
    }

    ml_json.get_to( ml );

    ml_json[ "originDomain" ].get_to( ml.originDomain );
    ml_json[ "requestedTarget" ].get_to( ml.requestedTarget );
    ml_json[ "name" ].get_to( ml.name );
  }
}

void OCI::Extensions::Dir::fetchManifest( Schema2::ImageManifest &im, Schema2::ImageManifest const &request ) {
  std::filesystem::directory_entry im_dir_path;
  std::filesystem::path im_file_path;

  if ( _tree_root == _directory ) {
    im_dir_path =
        std::filesystem::directory_entry( _tree_root.path() / request.originDomain /
                                          ( request.name + ":" + request.requestedTarget ) / request.requestedDigest );
  } else {
    im_dir_path = std::filesystem::directory_entry(
        _directory.path() / ( request.name + ":" + request.requestedTarget ) / request.requestedDigest );
  }

  im_file_path = im_dir_path.path() / "ImageManifest.json";

  if ( std::filesystem::exists( im_file_path ) ) {
    nlohmann::json im_json;
    auto read_file = false;

    do {
      if ( not read_file ) {
        try {
          std::ifstream im_file( im_file_path );
          im_file >> im_json;
          read_file = true;
        } catch ( nlohmann::detail::parse_error &e ) {
          spdlog::critical( "Fatal error '{}', while working on {}:{}/{}", e.what(), request.name, request.requestedTarget, request.requestedDigest );
          std::terminate();
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

  spdlog::error( "OCI::Extensions::Dir::putManifest Schema1::ImageManifest is not implemented" );

  return retVal;
}

auto OCI::Extensions::Dir::putManifest( Schema1::SignedImageManifest const &sim, std::string const &target ) -> bool {
  bool retVal = true;
  (void)sim;
  (void)target;

  spdlog::error( "OCI::Extensions::Dir::putManifest Schema1::SignedImageManifest is not implemented" );

  return retVal;
}

auto OCI::Extensions::Dir::putManifest( Schema2::ManifestList const &ml, [[maybe_unused]] std::string const &target )
    -> bool {
  bool retVal = false;
  auto manifest_list_dir_path =
      std::filesystem::directory_entry( _tree_root.path() / ml.originDomain / ( ml.name + ":" + ml.requestedTarget ) );
  auto manifest_list_path = manifest_list_dir_path.path() / "ManifestList.json";
  auto version_path = manifest_list_dir_path.path() / "Version";

  if ( manifest_list_dir_path.exists() ) {
    for ( auto const &file : std::filesystem::directory_iterator( manifest_list_dir_path ) ) {
      if ( file.is_directory() ) {
        auto file_str = file.path().filename().string();
        auto mlm_itr = std::find_if(
            ml.manifests.begin(), ml.manifests.end(),
            [ file_str ]( Schema2::ManifestList::Manifest const &mlm ) -> bool { return mlm.digest == file_str; } );

        if ( mlm_itr == ml.manifests.end() ) {
          std::lock_guard< std::mutex > lg( DIR_MUTEX );

          if ( file.exists() ) {
            spdlog::info( "OCI::Extensions::putManifest {} is not a ImageManifest of {}:{}", file.path().string(), ml.name, ml.requestedTarget );
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
  manifest_list_json[ "originDomain" ] = ml.originDomain;
  manifest_list_json[ "requestedTarget" ] = ml.requestedTarget;
  manifest_list_json[ "name" ] = ml.name;

  auto valid = true;
  for ( auto const &im : ml.manifests ) {
    auto im_path = manifest_list_dir_path.path() / im.digest / "ImageManifest.json";

    if ( not std::filesystem::exists( im_path ) ) {
      spdlog::error( "OCI::Extensions::putManifest Unable to write ManifestList, missing ImageManifest: \n {}", im_path.string() );
      valid = false;
    }
  }

  if ( valid and not std::filesystem::exists( manifest_list_path ) ) {
    spdlog::info( "OCI::Extensions::putManifest Schema2::ManifestList -> Writing file " );

    std::lock_guard< std::mutex > lg( DIR_MUTEX );
    std::ofstream manifest_list( manifest_list_path );
    std::ofstream version( version_path );

    manifest_list << std::setw( 2 ) << manifest_list_json;
    version << ml.schemaVersion;

    retVal = true;
  }

  return retVal;
} // OCI::Extensions::Dir::putManifest Schema2::ManifestList

auto OCI::Extensions::Dir::putManifest( Schema2::ImageManifest const &im, std::string &target ) -> bool {
  bool retVal = true;
  auto image_dir_path = std::filesystem::directory_entry( _tree_root.path() / im.originDomain /
                                                          ( im.name + ":" + im.requestedTarget ) / target );
  auto image_manifest_path = image_dir_path.path() / "ImageManifest.json";

  nlohmann::json image_manifest_json = im;

  // Output the Extensions for later reads
  image_manifest_json[ "originDomain" ] = im.originDomain;
  image_manifest_json[ "requestedTarget" ] = im.requestedTarget;
  image_manifest_json[ "requestedDigest" ] = im.requestedDigest;
  image_manifest_json[ "name" ] = im.name;

  for ( auto const &layer : im.layers ) {
    if ( not hasBlob( im, target, layer.digest ) ) {
      retVal = false;
    }
  }

  if ( retVal and hasBlob( im, target, im.config.digest ) ) {
    if ( std::filesystem::exists( image_manifest_path ) ) {
      retVal = false; // Because nothing changed
    } else {
      spdlog::info( "OCI::Extensions::putManifest Schema::ImageManifest -> Writing file" );
      std::ofstream image_manifest( image_manifest_path );

      image_manifest << std::setw( 2 ) << image_manifest_json;
    }
  } else {
    spdlog::error( "OCI::Extensions::Dir::putManifest Layer or Config digest missing for {}:{}/{}", im.name,
                   im.requestedTarget, im.requestedDigest );
    retVal = false;
  }

  return retVal;
}

auto OCI::Extensions::Dir::swap( Dir &other ) -> void {
  std::swap( _bytes_written, other._bytes_written );
  std::swap( _tree_root, other._tree_root );
  std::swap( _directory, other._directory );
  std::swap( _blobs_dir, other._blobs_dir );
  std::swap( _temp_dir, other._temp_dir );
}

auto OCI::Extensions::Dir::tagList( std::string const &rsrc ) -> OCI::Tags {
  OCI::Tags retVal;
  auto const &dir_map = dirMap(); // Can I efficiently replace this with a directory query

  if ( dir_map.find( rsrc ) != dir_map.end() ) {
    retVal.name = rsrc;
    retVal.tags = dir_map.at( rsrc ).tags;
  }

  return retVal;
}

auto OCI::Extensions::Dir::dirMap() -> DirMap const & {
  static std::map< std::string, DirMap > retVal;
  auto dir = _directory;

  // expecting dir to be root of the tree
  //  - subtree of namespaces
  //   - subtree of repo-name:(tag|digest) (contents varies based on Schema version)
  //    - subtree of digest with ImageManifests and blobs (Schemav2 only)
  // Top level can be either the root dir with domain subtrees or within a domain dir as tree root

  if ( retVal[ _directory.path().string() ].empty() ) {
    std::lock_guard< std::mutex > lg( DIR_MAP_MUT );

    if ( retVal[ _directory.path().string() ].empty() ) {
      spdlog::info( "OCI::Extensions::Dir::dirMap Generating Directory Map of: {}", _directory.path().string() );
      auto &dir_map = retVal[ _directory.path().string() ];
      auto base_dir = dir;

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
              auto tag = repo_str.substr( repo_str.find( ':' ) + 1 );
              auto repo_name = repo_str.substr( 0, repo_str.find( ':' ) );

              dir_map[ repo_name ].tags.push_back( tag );

              if ( std::filesystem::exists( path_part.path() / "ManifestList.json" ) ) {
                dir_map[ repo_name ].path[ tag ] = path_part;
              }
            } else {
              auto repo_name = repo_str.substr( 0, repo_str.find( ':' ) );
              auto target = repo_str.substr( repo_str.find_last_of( '/' ) + 1 );

              dir_map[ repo_name ].path[ target ] = path_part;
            }
          } else if ( repo_str.find( "sha" ) != std::string::npos and repo_str.find( "json" ) == std::string::npos ) {
            // Blobs
            auto repo_name = repo_str.substr( 0, repo_str.find( ':' ) );
            auto target = repo_str.substr( repo_str.find_last_of( '/' ) + 1 );

            dir_map[ repo_name ].path[ target ] = path_part;
          }
        }
      }
    }
  }

  return retVal[ _directory.path().string() ];
}
