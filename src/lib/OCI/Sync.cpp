#include <OCI/Sync.hpp>
#include <map>
#include <spdlog/spdlog.h>
#include <thread>
#include <vector>

OCI::Sync::Sync() : copier_( new Copy ), stm_( copier_->stm_ ), progress_bars_( copier_->progress_bars_ ) {}

void OCI::Sync::execute( OCI::Extensions::Yaml *src, OCI::Base::Client *dest ) {
  copier_->src_  = src;
  copier_->dest_ = dest;

  for ( auto const &domain : src->domains() ) {
    auto const catalog = src->catalog( domain );

    auto sync_bar_ref =
        progress_bars_->push_back( getIndicator( catalog.repositories.size(), domain, indicators::Color::cyan ) );

    auto                  catalog_total  = catalog.repositories.size();
    std::atomic< size_t > repo_thr_count = 0;
    auto                  repo_index     = 0;
    for ( auto const &repo : catalog.repositories ) {
      repo_thr_count++;

      stm_->background( [ &src, &repo_thr_count, &sync_bar_ref, &repo_index, &catalog_total, repo, this ]() {
        gobha::DelayedCall dec_count( [ &repo_thr_count, repo ]() {
          spdlog::trace( "OCI::Sync::execute '{}' finished decrementing count", repo );
          --repo_thr_count;
        } );
        execute( repo, src->copy()->tagList( repo ).tags );

        sync_bar_ref.get().tick();
        sync_bar_ref.get().set_option(
            indicators::option::PostfixText{ std::to_string( ++repo_index ) + "/" + std::to_string( catalog_total ) } );
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

void OCI::Sync::execute( OCI::Base::Client *src, OCI::Base::Client *dest ) {
  copier_->src_  = src;
  copier_->dest_ = dest;

  auto const catalog = src->catalog();
  auto       sync_bar_ref =
      progress_bars_->push_back( getIndicator( catalog.repositories.size(), "Source Repos", indicators::Color::cyan ) );

  auto                  catalog_total  = catalog.repositories.size();
  std::atomic< size_t > repo_thr_count = 0;
  auto                  repo_index     = 0;

  for ( auto const &repo : catalog.repositories ) {
    repo_thr_count++;

    stm_->background( [ &src, &repo_thr_count, &sync_bar_ref, &repo_index, &catalog_total, repo, this ]() {
      gobha::DelayedCall dec_count( [ &repo_thr_count, repo ]() {
        spdlog::trace( "OCI::Sync::execute '{}' finished decrementing count", repo );
        --repo_thr_count;
      } );

      execute( repo, src->copy()->tagList( repo ).tags );

      sync_bar_ref.get().tick();
      sync_bar_ref.get().set_option(
          indicators::option::PostfixText{ std::to_string( ++repo_index ) + "/" + std::to_string( catalog_total ) } );
    } );
  }

  while ( repo_thr_count != 0 ) {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for( 250ms );
  }

  spdlog::debug( "OCI::Sync::execute completed all repos" );
}

void OCI::Sync::execute( std::string const &rsrc ) { execute( rsrc, copier_->src_->copy()->tagList( rsrc ).tags ); }

void OCI::Sync::execute( std::string const &rsrc, std::vector< std::string > const &tags ) {
  auto sync_bar_ref = progress_bars_->push_back( getIndicator( tags.size(), rsrc, indicators::Color::magenta ) );
  auto total_tags   = tags.size();
  std::atomic< size_t > tag_index    = 0;
  std::atomic< size_t > thread_count = 0;

  for ( auto const &tag : tags ) {
    thread_count++;

    stm_->execute( [ &thread_count, &tag_index, &sync_bar_ref, rsrc, tag, total_tags, this ]() -> void {
      gobha::DelayedCall dec_count( [ &thread_count, rsrc, tag ]() {
        spdlog::trace( "OCI::Sync::execute '{}:{}' finished decrementing count", rsrc, tag );
        --thread_count;
      } );

      spdlog::trace( "OCI::Sync::execute -> forward to instance of OCI::Copy '{}:{}'", rsrc, tag );
      copier_->execute( rsrc, tag );

      sync_bar_ref.get().tick();
      sync_bar_ref.get().set_option(
          indicators::option::PostfixText{ std::to_string( ++tag_index ) + "/" + std::to_string( total_tags ) } );
    } );
  }

  while ( thread_count != 0 ) {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for( 250ms );
  }

  spdlog::debug( "OCI::Sync::execute completed all tags for '{}'", rsrc );
}
