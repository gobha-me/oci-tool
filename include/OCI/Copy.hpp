#pragma once
#include <string>
#include <OCI/Schema1.hpp>
#include <OCI/Schema2.hpp>


//  So what
//    classifies a SRC
//    classifies a DEST
//OCI::copy( rsrc , SRC, DEST ); // rsrc <namespace>/<repo><:<target>|none>
namespace OCI {
  template< class SRC, class DEST >
  void Copy( std::string rsrc, std::string target, SRC src, DEST dest ) {
    (void)dest;

    auto manifest_list = src.template manifest< OCI::Schema2::ManifestList >( rsrc, target );
    src.pull( manifest_list );
  }
} // namespace OCI
