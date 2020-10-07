#include <OCI/Extensions/Yaml.hpp>
#include <OCI/Factory.hpp>
#include <spdlog/spdlog.h>

OCI::Extensions::Yaml::Yaml( std::string const &file_path ) : _client( nullptr ) {
  ::Yaml::Node root_node; // need a new Yaml parser, this one doesn't follow C++ Iterator standards, which breaks range
                          // loops and the STL algorithms
  ::Yaml::Parse( root_node, file_path.c_str() ); // c-string for filename, std::string for a string to parse, WTF,
                                                 // should be two different functions

  for ( auto source_node = root_node.Begin(); source_node != root_node.End(); source_node++ ) {
    auto domain      = ( *source_node ).first;
    auto images_node = ( *source_node ).second[ "images" ];

    _catalog.domains.push_back( domain );
    _catalog.credentials[ domain ] = std::make_pair( ( *source_node ).second[ "username" ].As< std::string >(),
                                                     ( *source_node ).second[ "password" ].As< std::string >() );

    if ( ( *source_node ).second[ "architecture" ].IsSequence() ) {
      auto archs_node = ( *source_node ).second[ "architecture" ];

      for ( auto arch_node = archs_node.Begin(); arch_node != archs_node.End(); arch_node++ ) {
        _catalog.architectures.push_back( ( *arch_node ).second.As< std::string >() );
      }
    } else {
      _catalog.architectures.push_back( ( *source_node ).second[ "architecture" ].As< std::string >() );
    }
   
    _catalog.tag_filter = ( *source_node ).second[ "filter-tags" ].As< std::string >();

    for ( auto image_node = images_node.Begin(); image_node != images_node.End(); image_node++ ) {
      auto repo_name = ( *image_node ).first;

      _catalog.catalogs[ domain ].repositories.push_back( repo_name );

      if ( ( *image_node ).second.IsSequence() ) {
        _catalog.tags[ domain ][ repo_name ].name = repo_name;

        for ( auto tag_node = ( *image_node ).second.Begin(); tag_node != ( *image_node ).second.End(); tag_node++ ) {
          _catalog.tags[ domain ][ repo_name ].tags.push_back( ( *tag_node ).second.As< std::string >() );
        }
      }
    }
  }
}

OCI::Extensions::Yaml::Yaml( Yaml const &other ) {
  _catalog        = other._catalog;
  _current_domain = other._current_domain;

  if ( not _current_domain.empty() ) {
    _client = OCI::CLIENT_MAP.at( "docker" )( _current_domain, _catalog.credentials.at( _current_domain ).first,
                                            _catalog.credentials.at( _current_domain ).second );
  }
}

OCI::Extensions::Yaml::Yaml( Yaml &&other ) noexcept {
  _catalog        = std::move( other._catalog );
  _client         = std::move( other._client );
  _current_domain = std::move( other._current_domain );
}

auto OCI::Extensions::Yaml::operator=( Yaml const &other ) -> Yaml & {
  Yaml temp( other );
  *this = std::move( temp );

  return *this;
}

auto OCI::Extensions::Yaml::operator=( Yaml &&other ) noexcept -> Yaml & {
  _catalog        = std::move( other._catalog );
  _client         = std::move( other._client );
  _current_domain = std::move( other._current_domain );

  return *this;
}

auto OCI::Extensions::Yaml::catalog() -> const OCI::Catalog& {
  spdlog::error( "OCI::Extensions::Yaml is not a normal client, see documentation for details." );
  std::terminate();
}

auto OCI::Extensions::Yaml::catalog( std::string const &domain ) -> OCI::Catalog {
  _client         = OCI::CLIENT_MAP.at( "docker" )( domain, _catalog.credentials.at( domain ).first,
                                            _catalog.credentials.at( domain ).second );
  _current_domain = domain;

  for ( auto &repo : _catalog.catalogs.at( domain ).repositories ) {
    if ( repo.find( '/' ) == std::string::npos ) {
      repo.assign( "library/" + repo );
    }
  }

  return _catalog.catalogs.at( domain );
}

auto OCI::Extensions::Yaml::copy() -> std::unique_ptr< OCI::Base::Client > {
  return std::make_unique< OCI::Extensions::Yaml >( *this );
}

auto OCI::Extensions::Yaml::domains() const -> std::vector< std::string > const & { return _catalog.domains; }

auto OCI::Extensions::Yaml::fetchBlob( std::string const &rsrc, SHA256 sha,
                                       std::function< bool( const char *, uint64_t ) > &call_back ) -> bool {
  return _client->fetchBlob( rsrc, sha, call_back );
}

auto OCI::Extensions::Yaml::hasBlob( OCI::Schema1::ImageManifest const &im, SHA256 sha ) -> bool {
  return _client->hasBlob( im, sha );
}

auto OCI::Extensions::Yaml::hasBlob( OCI::Schema2::ImageManifest const &im, std::string const &target, SHA256 sha )
    -> bool {
  return _client->hasBlob( im, target, sha );
}

auto OCI::Extensions::Yaml::putBlob( OCI::Schema1::ImageManifest const &, std::string const &, std::uintmax_t, // NOLINT
                                     const char *, uint64_t ) -> bool {
  spdlog::error( "OCI::Extensions::Yaml is not a normal client, see documentation for details." );
  std::terminate();
}

auto OCI::Extensions::Yaml::putBlob( OCI::Schema2::ImageManifest const &, std::string const &, SHA256 const &, // NOLINT
                                     std::uintmax_t, const char *, uint64_t ) -> bool {
  spdlog::error( "OCI::Extensions::Yaml is not a normal client, see documentation for details." );
  std::terminate();
}

void OCI::Extensions::Yaml::fetchManifest( OCI::Schema1::ImageManifest &      im,
                                           OCI::Schema1::ImageManifest const &request ) {
  _client->fetchManifest( im, request );
}

void OCI::Extensions::Yaml::fetchManifest( OCI::Schema1::SignedImageManifest &      sim,
                                           OCI::Schema1::SignedImageManifest const &request ) {
  _client->fetchManifest( sim, request );
}

void OCI::Extensions::Yaml::fetchManifest( OCI::Schema2::ManifestList &ml, OCI::Schema2::ManifestList const &request ) {
  _client->fetchManifest( ml, request );
}

void OCI::Extensions::Yaml::fetchManifest( OCI::Schema2::ImageManifest &      im,
                                           OCI::Schema2::ImageManifest const &request ) {
  _client->fetchManifest( im, request );
}

auto OCI::Extensions::Yaml::putManifest( OCI::Schema1::ImageManifest const &, std::string const & ) // NOLINT
    -> bool {
  spdlog::error( "OCI::Extensions::Yaml is not a normal client, see documentation for details." );
  std::terminate();
}

auto OCI::Extensions::Yaml::putManifest( OCI::Schema1::SignedImageManifest const &, std::string const &target ) // NOLINT
    -> bool {
  spdlog::error( "OCI::Extensions::Yaml is not a normal client, see documentation for details." );
  std::terminate();
}

auto OCI::Extensions::Yaml::putManifest( OCI::Schema2::ManifestList const &, std::string const &target ) // NOLINT
    -> bool {
  spdlog::error( "OCI::Extensions::Yaml is not a normal client, see documentation for details." );
  std::terminate();
}

auto OCI::Extensions::Yaml::putManifest( OCI::Schema2::ImageManifest const &, std::string const &target ) -> bool { // NOLINT
  spdlog::error( "OCI::Extensions::Yaml is not a normal client, see documentation for details." );
  std::terminate();
}

std::mutex REGEX_MUTEX;
auto OCI::Extensions::Yaml::tagList( std::string const &rsrc ) -> OCI::Tags {
  OCI::Tags retVal;

  if ( _catalog.tags.find( _current_domain ) != _catalog.tags.end() and
       _catalog.tags.at( _current_domain ).find( rsrc ) != _catalog.tags.at( _current_domain ).end() ) {
    retVal = _catalog.tags.at( _current_domain ).at( rsrc );
  } else if ( _catalog.tag_filter.empty() ) {
    retVal = _client->tagList( rsrc );
  } else {
    std::lock_guard< std::mutex > lg( REGEX_MUTEX );
    std::regex tag_filter{ _catalog.tag_filter };
    retVal = _client->tagList( rsrc, tag_filter );
  }

  return retVal;
}

auto OCI::Extensions::Yaml::tagList( std::string const &rsrc, std::regex const & /*re*/ ) -> OCI::Tags {
  auto retVal = tagList( rsrc );

  return retVal;
}

auto OCI::Extensions::Yaml::swap( Yaml &other ) -> void {
  std::swap( _catalog, other._catalog );
  std::swap( _current_domain, other._current_domain );
  std::swap( _client, other._client );
}
