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
  class CountGuard {
  public:
    explicit CountGuard( std::atomic< size_t > &counter ) : _counter( counter ) {}

    CountGuard( CountGuard const & ) = delete;
    CountGuard( CountGuard && ) = delete;

    ~CountGuard() {
      --_counter.get();
    }

    auto operator=( CountGuard const & ) -> CountGuard & = delete;
    auto operator=( CountGuard && ) -> CountGuard & = delete;
  protected:
  private:
    std::reference_wrapper< std::atomic< size_t > > _counter;
  };

  class SimpleThreadManager {
  public:
    SimpleThreadManager() : SimpleThreadManager( std::thread::hardware_concurrency() * 2 ) {}

    explicit SimpleThreadManager( size_t thr_count ) : _capacity( thr_count ) {
      _thrs.reserve( _capacity );

      startManager();
    }

    SimpleThreadManager( SimpleThreadManager const& ) = delete;
    SimpleThreadManager( SimpleThreadManager && other ) noexcept {
      _capacity = other._capacity;
      _thrs     = std::move( other._thrs );
    }

    ~SimpleThreadManager() {
      _run_thr = false;

      if ( _started ) {
        for ( auto &thr : _thrs ) {
          thr.join();
        }
      }
    }

    auto operator=( SimpleThreadManager const& ) -> SimpleThreadManager& = delete;
    auto operator=( SimpleThreadManager && ) -> SimpleThreadManager& = delete;

    auto execute( std::function< void() >&& func ) -> void {
      bool enqueued{ false };

      {
        std::lock_guard< std::mutex > fg_lg( _fq_mutex );
        
        if ( _func_queue.size() < _capacity / 4 ) { // NOLINT
          enqueued = true;
          _func_queue.push_back( func );
          _cv.notify_all();
        }
      }

      if ( not enqueued ) {
        func();
      }
    }

    auto background( std::function< void() >&& func ) -> void {
      bool enqueued{ false };
      
      while ( not enqueued ) {
        std::lock_guard< std::mutex > fg_lg( _fq_mutex );
        
        if ( _func_queue.size() < _capacity / 4 ) { // NOLINT
          enqueued = true;
          _func_queue.push_back( func );
          _cv.notify_all();
        }
      }
    }
  protected:
    auto startManager() -> void {
      using namespace std::chrono_literals;

      if ( not _started ) {
        _started = true;

        for ( size_t thr_limit = 0; thr_limit != _thrs.capacity(); ++thr_limit ) {
          _thrs.emplace_back( [this]() {
              while ( _run_thr ) {
                std::function< void() > func;
                bool has_func = false;

                {
                  std::unique_lock< std::mutex > ul( _fq_mutex );
                  _cv.wait_for( ul, 250ms, [this]() { return _func_queue.empty(); } );

                  if ( not _func_queue.empty() ) {
                    has_func = true;
                    func = _func_queue.front();
                    _func_queue.pop_front();
                  }
                }

                if ( has_func ) {
                  func();
                }
              }
          } );
        }
      }
    }
  private:
    bool                                 _started{ false };
    std::atomic< bool >                  _run_thr{ true };
    size_t                               _capacity;
    std::vector< std::thread >           _thrs;
    std::list< std::function< void() > > _func_queue;
    std::condition_variable              _cv;
    std::mutex                           _fq_mutex;
  };
} // namespace gobha
