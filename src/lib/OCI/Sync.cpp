#include <OCI/Sync.hpp>

void OCI::Sync( const std::string& rsrc, OCI::Base::Client* src, OCI::Base::Client* dest ) {
  auto tagList = src->tagList( rsrc );

  for ( auto const& tag: tagList.tags ) {
    Copy( tagList.name, tag, src, dest );
  }
}

void OCI::Sync( const std::string& rsrc, const std::vector< std::string >& tags, OCI::Base::Client* src, OCI::Base::Client* dest ) {
  for ( auto const& tag: tags ) {
    Copy( rsrc, tag, src, dest );
  }
}
