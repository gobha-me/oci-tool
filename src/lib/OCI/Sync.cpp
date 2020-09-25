#include <OCI/Sync.hpp>
#include <thread>
#include <vector>
#include <map>
#include <spdlog/spdlog.h>

OCI::Sync::Sync() : _stm( std::make_shared< gobha::SimpleThreadManager >( gobha::SimpleThreadManager() ) ),
                    _progress_bars( std::make_shared< ProgressBars >( ProgressBars() ) ) {}

void OCI::Sync::execute( OCI::Extensions::Yaml* src, OCI::Base::Client* dest ) {
  _src  = src;
  _dest = dest;

  Copy copier{};

  for ( auto const& domain : src->domains() ) {
    auto const catalog = src->catalog( domain );

    auto indicator    = Copy::getIndicator( catalog.repositories.size(), domain, indicators::Color::cyan );
    auto sync_bar_ref = _progress_bars->push_back( indicator );

    auto                  catalog_total  = catalog.repositories.size();
    std::atomic< size_t > repo_thr_count = 0;
    auto                  repo_index     = 0;
    for ( auto const& repo : catalog.repositories ) {
      repo_thr_count++;

      _stm->execute( [&repo, &src, &repo_thr_count, &sync_bar_ref, &repo_index, &catalog_total, this]() {
        execute( repo, src->copy()->tagList( repo ).tags );

        sync_bar_ref.get().tick();
        sync_bar_ref.get().set_option( indicators::option::PostfixText{
      	  std::to_string( ++repo_index ) + "/" + std::to_string( catalog_total )
    	  } );
        repo_thr_count--;
      } );
    }

    while ( repo_thr_count != 0 ) {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for( 150ms );
    }
  }
}

void OCI::Sync::execute( OCI::Base::Client* src, OCI::Base::Client* dest ) {
  _src  = src;
  _dest = dest;

  auto const catalog = src->catalog();

  for ( auto const& repo : catalog.repositories ) {
    execute( repo, src->copy()->tagList( repo ).tags );
  }
}

void OCI::Sync::execute( std::string const& rsrc ) {
  execute( rsrc, _src->copy()->tagList( rsrc ).tags );
}

void OCI::Sync::execute( std::string const& rsrc, std::vector< std::string > const& tags ) {
  auto indicator    = Copy::getIndicator( tags.size(), rsrc, indicators::Color::magenta );
  auto sync_bar_ref = _progress_bars->push_back( indicator );
  auto                  total_tags   = tags.size();
  std::atomic< size_t > tag_index    = 0;
  std::atomic< size_t > thread_count = 0;

  for ( auto const& tag: tags ) {
    thread_count++;

    _stm->execute( [&rsrc, &tag, total_tags, &thread_count, &tag_index, &sync_bar_ref, this]() -> void {
      Copy copier{};

      copier._stm           = _stm;
      copier._progress_bars = _progress_bars;
      auto source           = _src->copy();
      auto destination      = _dest->copy();

      copier.execute( rsrc, tag, source.get(), destination.get() );

      sync_bar_ref.get().tick();
      sync_bar_ref.get().set_option( indicators::option::PostfixText{
        std::to_string( ++tag_index ) + "/" + std::to_string( total_tags )
      } );

      thread_count--; // FIXME: CREATE A GUARD FOR THIS - IF THREAD EXITS EARLY THIS WILL CAUSE AN INFINITE LOOP
    } );
  }

  while ( thread_count != 0 ) {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for( 150ms );
  }
}
