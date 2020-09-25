#pragma once
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <list>
#include <mutex>
#include <thread>
#include <vector>
#include <spdlog/spdlog.h>

namespace gobha {
  class SimpleThreadManager {
  public:
    SimpleThreadManager() {
      _thrs.reserve( std::thread::hardware_concurrency() * 2 );
    }

    explicit SimpleThreadManager( size_t thr_count ) {
      _thrs.reserve( thr_count );
    }

    SimpleThreadManager( SimpleThreadManager const& ) = delete;
    SimpleThreadManager( SimpleThreadManager && other ) noexcept {
      _completed_thrs = std::move( other._completed_thrs );
      _thrs           = std::move( other._thrs );
    }

    ~SimpleThreadManager() {
      _run_thr = false;

      if ( _started ) {
        _managing_thr.join();

        for ( auto &thr : _thrs ) {
          thr.join();
        }
      }
    }

    auto operator=( SimpleThreadManager const& ) -> SimpleThreadManager& = delete;
    auto operator=( SimpleThreadManager && ) -> SimpleThreadManager& = delete;

    auto execute( std::function< void() >&& func ) -> void {
      startManager();

       bool enqueued{ false };

      {
        std::lock_guard< std::mutex > thrs_lg( _thrs_mutex );
        
        if ( _thrs.size() < _thrs.capacity() ) {
          enqueued = true;
          std::lock_guard< std::mutex > fg_lg( _fq_mutex );
          _func_queue.push_back( func );
        }
      }

      if ( not enqueued ) {
        func();
      }
    }
  protected:
    auto startManager() -> void {
      using namespace std::chrono_literals;

      if ( not _started ) {
        std::lock_guard< std::mutex > lg( _thrs_mutex );

        if ( not _started ) {
          _started = true;

          _managing_thr = std::thread( [this]() {
              while ( _run_thr ) {
                { // spawn thread
                  std::lock_guard< std::mutex > thrs_lg( _thrs_mutex );
                  std::unique_lock< std::mutex > ul( _fq_mutex );

                  if ( not _func_queue.empty() and _thrs.size() < _thrs.capacity() ) {
                    _thrs.emplace_back( [this]() {
                        std::function< void() > func;
                        bool has_func = false;

                        {
                          std::lock_guard< std::mutex > lg( _fq_mutex );
                          if ( not _func_queue.empty() ) {
                            has_func = true;
                            func = _func_queue.front();
                            _func_queue.pop_front();
                          }
                          _cv.notify_all();
                        }

                        if ( has_func ) {
                          func();
                        }

                        std::lock_guard< std::mutex > lg( _c_thrs_mutex );
                        _completed_thrs.push_back( std::this_thread::get_id() );
                        _cv.notify_all();
                      } );

                    _cv.wait_for( ul, 250ms, []() { return true; } );
                  }
                }

                { // cleanup
                  std::unique_lock< std::mutex > ul( _c_thrs_mutex );

                  _cv.wait_for( ul, 250ms, []() { return true; } );
                  auto thr_id_itr = std::find_if( _completed_thrs.begin(), _completed_thrs.end(), [this]( std::thread::id thr_id ) -> bool {
                      bool retVal = false;

                      std::lock_guard< std::mutex > thrs_lg( _thrs_mutex );
                      auto thr_itr = std::find_if( _thrs.begin(), _thrs.end(), [&] ( std::thread &thr) -> bool { return thr.get_id() == thr_id; } );

                      if ( thr_itr != _thrs.end() ) {
                        thr_itr->join();
                        _thrs.erase( thr_itr );
                        retVal = true;
                      }

                      return retVal;
                  } );

                  if ( thr_id_itr != _completed_thrs.end() ) {
                    _completed_thrs.erase( thr_id_itr );
                  }
                }
              }
            } );
        }
      }
    }
  private:
    bool                                   _started{ false };
    std::atomic< bool >                    _run_thr{ true };
    std::thread                            _managing_thr;
    std::vector< std::thread >             _thrs;
    std::vector< std::thread::id >         _completed_thrs;
    std::list< std::function< void() > >   _func_queue;
    std::condition_variable                _cv;
    std::mutex                             _c_thrs_mutex;
    std::mutex                             _thrs_mutex;
    std::mutex                             _fq_mutex;
  };
} // namespace gobha
