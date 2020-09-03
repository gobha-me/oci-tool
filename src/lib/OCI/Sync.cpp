#include <OCI/Sync.hpp>
#include <thread>
#include <vector>
#include <spdlog/spdlog.h>

void OCI::Sync( OCI::Extensions::Yaml* src, OCI::Base::Client* dest, indicators::DynamicProgress< indicators::ProgressBar >& progress_bars ) {
  constexpr auto THREAD_LIMIT = 4;
  std::vector< std::thread > processes;

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
        Sync( repo, src->tagList( repo ).tags, src, dest, progress_bars );
      } );

      while ( processes.size() == processes.capacity() ) {
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

      //auto &sync_bar_ref = progress_bars[ sync_bar_index ];
      sync_bar_ref.get().tick();
      sync_bar_ref.get().set_option( indicators::option::PostfixText{
      	std::to_string( ++repo_index ) + "/" + std::to_string( catalog.repositories.size() )
    	});
    }

    while ( not processes.empty() ) {
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
  constexpr auto THREAD_LIMIT=4;
  std::vector< std::thread > processes;

  processes.reserve( THREAD_LIMIT );

  spdlog::error( rsrc );
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

  //auto sync_bar_index = progress_bars.push_back( sync_bar );
  auto sync_bar_ref = progress_bars.push_back( sync_bar );

  auto tag_index = 0;
  for ( auto const& tag: tags ) {
    processes.emplace_back( [&]() -> void {
      Copy( rsrc, tag, src, dest );
    } );

    while ( processes.size() == processes.capacity() ) {
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

    //auto &sync_bar = progress_bars[ sync_bar_index ];
    sync_bar_ref.get().tick();
    sync_bar_ref.get().set_option( indicators::option::PostfixText{
      std::to_string( ++tag_index ) + "/" + std::to_string( tags.size() )
    });
  }

  for ( auto& process: processes ) {
    process.join();
  }
}
