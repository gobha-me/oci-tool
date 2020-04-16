#pragma once
#include <OCI/Base/Client.hpp>
#include <OCI/Copy.hpp>

namespace OCI {
  void Sync( const std::string& rsrc, OCI::Base::Client* src, OCI::Base::Client* dest );

  void Sync( const std::string& rsrc, std::vector< std::string > tags, OCI::Base::Client* src, OCI::Base::Client* dest );
} // namespace OCI


// Implementatiion
void OCI::Sync( const std::string& rsrc, OCI::Base::Client* src, OCI::Base::Client* dest ) {
  auto tagList = src->tagList( rsrc );

  for ( auto tag: tagList.tags )
    Copy( tagList.name, tag, src, dest );
}

void OCI::Sync( const std::string& rsrc, std::vector< std::string > tags, OCI::Base::Client* src, OCI::Base::Client* dest ) {
  for ( auto tag: tags )
    Copy( rsrc, tag, src, dest );
}
