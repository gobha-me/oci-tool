#include <OCI/Sync.hpp>
#include <thread>
#include <vector>
#include <map>
#include <spdlog/spdlog.h>

OCI::Sync::Sync() : _copier( new Copy ), _stm( _copier->_stm ), _progress_bars( _copier->_progress_bars ) {}

void OCI::Sync::execute( OCI::Extensions::Yaml* src, OCI::Base::Client* dest ) {
  _copier->_src  = src;
  _copier->_dest = dest;

  for ( auto const& domain : src->domains() ) {
    auto const catalog = src->catalog( domain );

//    auto indicator    = getIndicator( catalog.repositories.size(), domain, indicators::Color::cyan );
    auto sync_bar_ref = _progress_bars->push_back( getIndicator( catalog.repositories.size(), domain, indicators::Color::cyan ) );

    auto                  catalog_total  = catalog.repositories.size();
    std::atomic< size_t > repo_thr_count = 0;
    auto                  repo_index     = 0;
    for ( auto const& repo : catalog.repositories ) {
      repo_thr_count++;

      _stm->background( [&src, &repo_thr_count, &sync_bar_ref, &repo_index, &catalog_total, repo, this]() {
        gobha::DelayedCall dec_count( [&repo_thr_count, repo]() {
            spdlog::trace( "OCI::Sync::execute '{}' finished decrementing count", repo );
            --repo_thr_count;
          } );
        execute( repo, src->copy()->tagList( repo ).tags );

        sync_bar_ref.get().tick();
        sync_bar_ref.get().set_option( indicators::option::PostfixText{
      	  std::to_string( ++repo_index ) + "/" + std::to_string( catalog_total )
    	  } );
      } );
      std::this_thread::yield();
    }

    while ( repo_thr_count != 0 ) {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for( 250ms );
    }

    spdlog::debug( "OCI::Sync::execute completed all repos for '{}'", domain );
  }
}

void OCI::Sync::execute( OCI::Base::Client* src, OCI::Base::Client* dest ) {
  _copier->_src  = src;
  _copier->_dest = dest;

  auto const catalog = src->catalog();
//  auto indicator     = getIndicator( catalog.repositories.size(), "Source Repos", indicators::Color::cyan );
  auto sync_bar_ref  = _progress_bars->push_back( getIndicator( catalog.repositories.size(), "Source Repos", indicators::Color::cyan ) );

  auto                  catalog_total  = catalog.repositories.size();
  std::atomic< size_t > repo_thr_count = 0;
  auto                  repo_index     = 0;

  for ( auto const& repo : catalog.repositories ) {
    repo_thr_count++;

    _stm->background( [&src, &repo_thr_count, &sync_bar_ref, &repo_index, &catalog_total, repo, this]() {
      gobha::DelayedCall dec_count( [&repo_thr_count, repo]() {
          spdlog::trace( "OCI::Sync::execute '{}' finished decrementing count", repo );
          --repo_thr_count;
        } );

      execute( repo, src->copy()->tagList( repo ).tags );

      sync_bar_ref.get().tick();
      sync_bar_ref.get().set_option( indicators::option::PostfixText{
      	std::to_string( ++repo_index ) + "/" + std::to_string( catalog_total )
    	} );
    } );
  }

  while ( repo_thr_count != 0 ) {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for( 250ms );
  }

  spdlog::debug( "OCI::Sync::execute completed all repos" );
}

void OCI::Sync::execute( std::string const& rsrc ) {
  execute( rsrc, _copier->_src->copy()->tagList( rsrc ).tags );
}

void OCI::Sync::execute( std::string const& rsrc, std::vector< std::string > const& tags ) {
//  auto indicator    = getIndicator( tags.size(), rsrc, indicators::Color::magenta );
  auto sync_bar_ref = _progress_bars->push_back( getIndicator( tags.size(), rsrc, indicators::Color::magenta ) );
  auto                  total_tags   = tags.size();
  std::atomic< size_t > tag_index    = 0;
  std::atomic< size_t > thread_count = 0;

  for ( auto const& tag: tags ) {
    thread_count++;

    _stm->execute( [ &thread_count, &tag_index, &sync_bar_ref, rsrc, tag, total_tags, this ]() -> void {
      gobha::DelayedCall dec_count( [ &thread_count, rsrc ]() {
          spdlog::trace( "OCI::Sync::execute '{}' finished decrementing count", rsrc );
          --thread_count;
        } );

      spdlog::trace( "OCI::Sync::execute -> forward to instance of OCI::Copy '{}:{}'", rsrc, tag );
      _copier->execute( rsrc, tag );

      sync_bar_ref.get().tick();
      sync_bar_ref.get().set_option( indicators::option::PostfixText{
        std::to_string( ++tag_index ) + "/" + std::to_string( total_tags )
      } );
    } );
  }

  while ( thread_count != 0 ) {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for( 250ms );
  }

  spdlog::debug( "OCI::Sync::execute completed all tags for '{}'", rsrc );
}
