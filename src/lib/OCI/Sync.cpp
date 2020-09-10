#include <OCI/Sync.hpp>
#include <thread>
#include <vector>
#include <map>
#include <spdlog/spdlog.h>

void OCI::Sync( OCI::Extensions::Yaml* src, OCI::Base::Client* dest, indicators::DynamicProgress< indicators::ProgressBar >& progress_bars ) {
  constexpr auto THREAD_LIMIT = 10;
  std::vector< std::thread > processes;
  std::vector< std::thread::id > finished_threads;
  std::mutex vft_lock;

  processes.reserve( THREAD_LIMIT );

  for ( auto const& domain : src->domains() ) {
    auto const catalog = src->catalog( domain );

    // clang-format off
    indicators::ProgressBar sync_bar{
        indicators::option::BarWidth{80}, // NOLINT
    		indicators::option::Start{"["},
    		indicators::option::Fill{"■"},
    		indicators::option::Lead{"■"},
    		indicators::option::Remainder{" "},
    		indicators::option::End{" ]"},
        indicators::option::MaxProgress{ catalog.repositories.size() },
        indicators::option::ForegroundColor{indicators::Color::cyan},
        indicators::option::PrefixText{ domain },
        indicators::option::PostfixText{ "0/" + std::to_string( catalog.repositories.size() ) }
    };
    // clang-format on

    //auto sync_bar_index = progress_bars.push_back( sync_bar );
    auto sync_bar_ref = progress_bars.push_back( sync_bar );

    auto repo_index = 0;
    for ( auto const& repo : catalog.repositories ) {
      processes.emplace_back( [&]() -> void {
        Sync( repo, src->copy()->tagList( repo ).tags, src, dest, progress_bars );

        std::lock_guard< std::mutex > lg( vft_lock );
        finished_threads.push_back( std::this_thread::get_id() );
      } );

      while ( processes.size() == processes.capacity() ) {
        auto proc_itr = std::find_if( processes.begin(), processes.end(), [&]( std::thread& process ) {
            bool retVal = false;

            {
              std::lock_guard< std::mutex > lg( vft_lock );
              auto thr_id_itr = std::find( finished_threads.begin(), finished_threads.end(), process.get_id() );

              if ( thr_id_itr != finished_threads.end() ) {
                finished_threads.erase( thr_id_itr );
                retVal = true;
              }
            }

            if ( retVal ) {
              process.join();
            }

            return retVal;
        } );

        if ( proc_itr != processes.end() ) {
          processes.erase( proc_itr );

          sync_bar_ref.get().tick();
          sync_bar_ref.get().set_option( indicators::option::PostfixText{
      	    std::to_string( ++repo_index ) + "/" + std::to_string( catalog.repositories.size() )
    	    });
        } else {
          using namespace std::chrono_literals;
          std::this_thread::sleep_for( 250ms );
        }
      }
    }

    while ( not processes.empty() ) {
      auto proc_itr = std::find_if( processes.begin(), processes.end(), [&]( std::thread& process ) {
          bool retVal = false;

          {
            std::lock_guard< std::mutex > lg( vft_lock );
            auto thr_id_itr = std::find( finished_threads.begin(), finished_threads.end(), process.get_id() );

            if ( thr_id_itr != finished_threads.end() ) {
              finished_threads.erase( thr_id_itr );
              retVal = true;
            }
          }

          if ( retVal ) {
            process.join();
          }

          return retVal;
      } );

      if ( proc_itr != processes.end() ) {
        processes.erase( proc_itr );

        sync_bar_ref.get().tick();
        sync_bar_ref.get().set_option( indicators::option::PostfixText{
      	  std::to_string( ++repo_index ) + "/" + std::to_string( catalog.repositories.size() )
    	  });
      }
    }
  }
}

void OCI::Sync( OCI::Base::Client* src, OCI::Base::Client* dest, indicators::DynamicProgress< indicators::ProgressBar >& progress_bars ) {
  auto const catalog = src->catalog();

  for ( auto const& repo : catalog.repositories ) {
    Sync( repo, src->tagList( repo ).tags, src, dest, progress_bars );
  }
}

void OCI::Sync( std::string const& rsrc, OCI::Base::Client* src, OCI::Base::Client* dest, indicators::DynamicProgress< indicators::ProgressBar >& progress_bars ) {
  auto const tagList = src->tagList( rsrc );

  Sync( rsrc, tagList.tags, src, dest, progress_bars );
}

void OCI::Sync( std::string const& rsrc, std::vector< std::string > const& tags, OCI::Base::Client* src, OCI::Base::Client* dest, indicators::DynamicProgress< indicators::ProgressBar >& progress_bars ) {
  constexpr auto THREAD_LIMIT = 2;
  std::vector< std::thread > processes;
  std::vector< std::thread::id > finished_threads;
  std::mutex vft_lock;

  processes.reserve( THREAD_LIMIT );

  // clang-format off
  indicators::ProgressBar sync_bar{
      indicators::option::BarWidth{60}, // NOLINT
      indicators::option::Start{"["},
      indicators::option::Fill{"■"},
      indicators::option::Lead{"■"},
      indicators::option::Remainder{" "},
      indicators::option::End{" ]"},
      indicators::option::MaxProgress{ tags.size() },
      indicators::option::ForegroundColor{indicators::Color::magenta},
      indicators::option::PrefixText{ rsrc },
      indicators::option::PostfixText{ "0/" + std::to_string( tags.size() ) }
    };
    // clang-format on

  auto sync_bar_ref = progress_bars.push_back( sync_bar );

  auto tag_index = 0;
  for ( auto const& tag: tags ) {
    processes.emplace_back( [&]() -> void {
      Copy( rsrc, tag, src, dest, progress_bars );

      std::lock_guard< std::mutex > lg( vft_lock );
      finished_threads.push_back( std::this_thread::get_id() );
    } );

    while ( processes.size() == processes.capacity() ) {
      auto proc_itr = std::find_if( processes.begin(), processes.end(), [&]( std::thread& process ) {
          bool retVal = false;

          {
            std::lock_guard< std::mutex > lg( vft_lock );
            auto thr_id_itr = std::find( finished_threads.begin(), finished_threads.end(), process.get_id() );

            if ( thr_id_itr != finished_threads.end() ) {
              finished_threads.erase( thr_id_itr );
              retVal = true;
            }
          }

          if ( retVal ) {
            process.join();
          }

          return retVal;
      } );

      if ( proc_itr != processes.end() ) {
        processes.erase( proc_itr );

        sync_bar_ref.get().tick();
        sync_bar_ref.get().set_option( indicators::option::PostfixText{
          std::to_string( ++tag_index ) + "/" + std::to_string( tags.size() )
        });
      } else {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for( 250ms );
      }
    }
  }

  for ( auto& process: processes ) {
    process.join();

    sync_bar_ref.get().tick();
    sync_bar_ref.get().set_option( indicators::option::PostfixText{
      std::to_string( ++tag_index ) + "/" + std::to_string( tags.size() )
    });
  }
}
