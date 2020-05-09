#include <OCI/Sync.hpp>
#include <thread>
#include <vector>

void OCI::Sync( OCI::Base::Client* src, OCI::Base::Client* dest ) {
  auto const& catalog = src->catalog();

  for ( auto const& repo : catalog.repositories ) {
    Sync( repo, src->tagList( repo ).tags , src, dest );
  }
}

void OCI::Sync( std::string const& rsrc, OCI::Base::Client* src, OCI::Base::Client* dest ) {
  auto const& tagList = src->tagList( rsrc );

  Sync( rsrc, tagList.tags, src, dest );
}

void OCI::Sync( std::string const& rsrc, std::vector< std::string > const& tags, OCI::Base::Client* src, OCI::Base::Client* dest ) {
  constexpr auto THREAD_LIMIT=8;
  std::vector< std::thread > processes;

  processes.reserve( THREAD_LIMIT );

  for ( auto const& tag: tags ) {
    processes.emplace_back( [&]() {
    //auto source      = src->copy();
    //auto destination = dest->copy();

    Copy( rsrc, tag, src, dest );
    } );

    if ( processes.size() == processes.capacity() ) {
      auto proc_itr = std::find_if( processes.begin(), processes.end(), []( std::thread& process ) {
          bool retVal = false;

          if ( process.joinable() ) {
            process.join();
            retVal = true;
          }

          return retVal;
      } );

      if ( proc_itr != processes.end() ) {
        processes.erase( proc_itr );
      }
    }
  }

  for ( auto& process: processes ) {
    process.join();
  }
}
