#include <OCI/Sync.hpp>

void OCI::Sync( OCI::Base::Client* src, OCI::Base::Client* dest ) {
  auto const& catalog = src->catalog();

  for ( auto const& repo : catalog.repositories ) {
    std::cout << repo << std::endl;
    auto const& tagList = src->tagList( repo );

    Sync( repo, tagList.tags, src, dest );
  }
}

void OCI::Sync( std::string const& rsrc, OCI::Base::Client* src, OCI::Base::Client* dest ) {
  auto const& tagList = src->tagList( rsrc );

  Sync( rsrc, tagList.tags, src, dest );
}

void OCI::Sync( std::string const& rsrc, std::vector< std::string > const& tags, OCI::Base::Client* src, OCI::Base::Client* dest ) {
  for ( auto const& tag: tags ) {
    Copy( rsrc, tag, src, dest );
  }
}
