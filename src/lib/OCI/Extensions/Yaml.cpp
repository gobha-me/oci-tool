#include <OCI/Extensions/Yaml.hpp>
#include <OCI/Factory.hpp>
#include <spdlog/spdlog.h>

OCI::Extensions::Yaml::Yaml( std::string const &file_path ) : client_( nullptr ) {
  ::Yaml::Node root_node; // need a new Yaml parser, this one doesn't follow C++ Iterator standards, which breaks range
                          // loops and the STL algorithms
  ::Yaml::Parse( root_node, file_path.c_str() ); // c-string for filename, std::string for a string to parse, WTF,
                                                 // should be two different functions

  for ( auto source_node = root_node.Begin(); source_node != root_node.End(); source_node++ ) {
    auto domain      = ( *source_node ).first;
    auto images_node = ( *source_node ).second[ "images" ];

    catalog_.domains.push_back( domain );
    catalog_.credentials[ domain ] = std::make_pair( ( *source_node ).second[ "username" ].As< std::string >(),
                                                     ( *source_node ).second[ "password" ].As< std::string >() );

    if ( ( *source_node ).second[ "architecture" ].IsSequence() ) {
      auto archs_node = ( *source_node ).second[ "architecture" ];

      for ( auto arch_node = archs_node.Begin(); arch_node != archs_node.End(); arch_node++ ) {
        catalog_.architectures.push_back( ( *arch_node ).second.As< std::string >() );
      }
    } else {
      catalog_.architectures.push_back( ( *source_node ).second[ "architecture" ].As< std::string >() );
    }

    if ( ( *source_node ).second[ "tag-options" ].IsMap() ) {
      catalog_.tag_options.filter = ( *source_node ).second[ "tag-options" ][ "filter" ].As< std::string >();
      catalog_.tag_options.limit  = ( *source_node ).second[ "tag-options" ][ "limit" ].As< uint16_t >();
    }

    for ( auto image_node = images_node.Begin(); image_node != images_node.End(); image_node++ ) {
      auto repo_name = ( *image_node ).first;

      catalog_.catalogs[ domain ].repositories.push_back( repo_name );

      if ( ( *image_node ).second.IsSequence() ) {
        catalog_.tags[ domain ][ repo_name ].name = repo_name;

        for ( auto tag_node = ( *image_node ).second.Begin(); tag_node != ( *image_node ).second.End(); tag_node++ ) {
          catalog_.tags[ domain ][ repo_name ].tags.push_back( ( *tag_node ).second.As< std::string >() );
        }
      }
    }
  }
}

OCI::Extensions::Yaml::Yaml( Yaml const &other ) {
  catalog_        = other.catalog_;
  current_domain_ = other.current_domain_;

  if ( not current_domain_.empty() ) {
    client_ = OCI::CLIENT_MAP.at( "docker" )( current_domain_, catalog_.credentials.at( current_domain_ ).first,
                                              catalog_.credentials.at( current_domain_ ).second );
  }
}

OCI::Extensions::Yaml::Yaml( Yaml &&other ) noexcept {
  catalog_        = std::move( other.catalog_ );
  client_         = std::move( other.client_ );
  current_domain_ = std::move( other.current_domain_ );
}

auto OCI::Extensions::Yaml::operator=( Yaml const &other ) -> Yaml & {
  Yaml temp( other );
  *this = std::move( temp );

  return *this;
}

auto OCI::Extensions::Yaml::operator=( Yaml &&other ) noexcept -> Yaml & {
  catalog_        = std::move( other.catalog_ );
  client_         = std::move( other.client_ );
  current_domain_ = std::move( other.current_domain_ );

  return *this;
}

auto OCI::Extensions::Yaml::catalog() -> const OCI::Catalog & {
  spdlog::error( "OCI::Extensions::Yaml is not a normal client, see documentation for details." );
  std::terminate();
}

auto OCI::Extensions::Yaml::catalog( std::string const &domain ) -> OCI::Catalog {
  client_         = OCI::CLIENT_MAP.at( "docker" )( domain, catalog_.credentials.at( domain ).first,
                                            catalog_.credentials.at( domain ).second );
  current_domain_ = domain;

  for ( auto &repo : catalog_.catalogs.at( domain ).repositories ) {
    if ( repo.find( '/' ) == std::string::npos ) {
      repo.assign( "library/" + repo );
    }
  }

  return catalog_.catalogs.at( domain );
}

auto OCI::Extensions::Yaml::copy() -> std::unique_ptr< OCI::Base::Client > {
  return std::make_unique< OCI::Extensions::Yaml >( *this );
}

auto OCI::Extensions::Yaml::domains() const -> std::vector< std::string > const & { return catalog_.domains; }

auto OCI::Extensions::Yaml::fetchBlob( std::string const &rsrc, SHA256 sha,
                                       std::function< bool( const char *, uint64_t ) > &call_back ) -> bool {
  return client_->fetchBlob( rsrc, sha, call_back );
}

auto OCI::Extensions::Yaml::hasBlob( OCI::Schema1::ImageManifest const &im, SHA256 sha ) -> bool {
  return client_->hasBlob( im, sha );
}

auto OCI::Extensions::Yaml::hasBlob( OCI::Schema2::ImageManifest const &im, std::string const &target, SHA256 sha )
    -> bool {
  return client_->hasBlob( im, target, sha );
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
  client_->fetchManifest( im, request );
}

void OCI::Extensions::Yaml::fetchManifest( OCI::Schema1::SignedImageManifest &      sim,
                                           OCI::Schema1::SignedImageManifest const &request ) {
  client_->fetchManifest( sim, request );
}

void OCI::Extensions::Yaml::fetchManifest( OCI::Schema2::ManifestList &ml, OCI::Schema2::ManifestList const &request ) {
  client_->fetchManifest( ml, request );
}

void OCI::Extensions::Yaml::fetchManifest( OCI::Schema2::ImageManifest &      im,
                                           OCI::Schema2::ImageManifest const &request ) {
  client_->fetchManifest( im, request );
}

auto OCI::Extensions::Yaml::putManifest( OCI::Schema1::ImageManifest const &, std::string const & ) // NOLINT
    -> bool {
  spdlog::error( "OCI::Extensions::Yaml is not a normal client, see documentation for details." );
  std::terminate();
}

auto OCI::Extensions::Yaml::putManifest( OCI::Schema1::SignedImageManifest const &,
                                         std::string const &target ) // NOLINT
    -> bool {
  spdlog::error( "OCI::Extensions::Yaml is not a normal client, see documentation for details." );
  std::terminate();
}

auto OCI::Extensions::Yaml::putManifest( OCI::Schema2::ManifestList const &, std::string const &target ) // NOLINT
    -> bool {
  spdlog::error( "OCI::Extensions::Yaml is not a normal client, see documentation for details." );
  std::terminate();
}

auto OCI::Extensions::Yaml::putManifest( OCI::Schema2::ImageManifest const &, std::string const &target )
    -> bool { // NOLINT
  spdlog::error( "OCI::Extensions::Yaml is not a normal client, see documentation for details." );
  std::terminate();
}

std::mutex REGEX_MUTEX;
auto       OCI::Extensions::Yaml::tagList( std::string const &rsrc ) -> OCI::Tags {
  OCI::Tags retVal;

  // FIXME? without seperate method for tag list limiting would need to implement here
  if ( catalog_.tags.find( current_domain_ ) != catalog_.tags.end() and
       catalog_.tags.at( current_domain_ ).find( rsrc ) != catalog_.tags.at( current_domain_ ).end() ) {
    retVal = catalog_.tags.at( current_domain_ ).at( rsrc );
  } else if ( catalog_.tag_options.filter.empty() ) {
    retVal = client_->tagList( rsrc );
  } else {
    std::lock_guard< std::mutex > lg( REGEX_MUTEX );
    std::regex                    tag_filter{ catalog_.tag_options.filter };
    retVal = client_->tagList( rsrc, tag_filter );
  }

  return retVal;
}

auto OCI::Extensions::Yaml::tagList( std::string const &rsrc, std::regex const & /*re*/ ) -> OCI::Tags {
  auto retVal = tagList( rsrc );

  return retVal;
}

auto OCI::Extensions::Yaml::swap( Yaml &other ) -> void {
  std::swap( catalog_, other.catalog_ );
  std::swap( current_domain_, other.current_domain_ );
  std::swap( client_, other.client_ );
}
