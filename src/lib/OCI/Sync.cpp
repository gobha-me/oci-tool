#include <OCI/Sync.hpp>
#include <future>
#include <vector>

void OCI::Sync( OCI::Base::Client* src, OCI::Base::Client* dest ) {
  auto const& catalog = src->catalog();

  for ( auto const& repo : catalog.repositories ) {
    auto const& tagList = src->tagList( repo );

    Sync( repo, tagList.tags, src, dest );
  }
}

void OCI::Sync( std::string const& rsrc, OCI::Base::Client* src, OCI::Base::Client* dest ) {
  auto const& tagList = src->tagList( rsrc );

  Sync( rsrc, tagList.tags, src, dest );
}

void OCI::Sync( std::string const& rsrc, std::vector< std::string > const& tags, OCI::Base::Client* src, OCI::Base::Client* dest ) {
  std::vector< std::future< void > > processes;

  processes.reserve( tags.size() );

  for ( auto const& tag: tags ) {
    processes.push_back( std::async( std::launch::async, [&]() {
      Copy( rsrc, tag, src, dest );
    } ) );
  }

  for ( auto& process : processes ) {
    process.wait();
  }
}
