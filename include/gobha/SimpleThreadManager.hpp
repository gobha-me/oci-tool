#pragma once
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <list>
#include <mutex>
#include <thread>
#include <vector>
#include <queue>
#include <spdlog/spdlog.h>

namespace gobha {
  class CountGuard {
  public:
    explicit CountGuard( std::atomic< size_t > &counter ) : counter_( counter ) {}

    CountGuard( CountGuard const & ) = delete;
    CountGuard( CountGuard && ) = delete;

    ~CountGuard() {
      --counter_.get();
    }

    auto operator=( CountGuard const & ) -> CountGuard & = delete;
    auto operator=( CountGuard && ) -> CountGuard & = delete;
  protected:
  private:
    std::reference_wrapper< std::atomic< size_t > > counter_;
  };

  class DelayedCall {
  public:
    explicit DelayedCall( std::function< void() > &&func ) : func_( func ) {}
    DelayedCall( DelayedCall const & ) = delete;
    DelayedCall( DelayedCall && ) = delete;
    ~DelayedCall() {
      spdlog::trace( "gobha::DelayedCall left scope calling function" );
      func_();
    }

    auto operator=( DelayedCall const& ) = delete;
    auto operator=( DelayedCall && ) = delete;
  protected:
  private:
    std::function< void() > func_;
  };

  class SimpleThreadManager {
  public:
    SimpleThreadManager() {
      using namespace std::chrono_literals;

      for ( size_t thr_limit = 0; thr_limit != thread_limit_; ++thr_limit ) {
        thrs_.emplace_back( [ this ]() {
          spdlog::trace( "gobha::SimpleThreadManager Thread worker starting" );

          while ( run_thr_ ) {
            std::function< void() > func;

            {
              std::unique_lock< std::mutex > ul( q_mut_ );
              cv_.wait_for( ul, 250ms, [this]() -> bool { return not bg_funcs_.empty() or not fg_funcs_.empty(); } );

              if ( ul.owns_lock() ) {
                if ( not fg_funcs_.empty() ) {
                  func = fg_funcs_.front();

                  fg_funcs_.pop_front();
                } else if ( not bg_funcs_.empty() ) {
                  func = bg_funcs_.front();

                  bg_funcs_.pop_front();
                }
              }

              cv_.notify_one();
            }

            if ( func != nullptr ) {
              idle_--;
              func();
              idle_++;
            }
          }

          spdlog::trace( "gobha::SimpleThreadManager Thread worker closing" );
        } );
      }
    }

    SimpleThreadManager( SimpleThreadManager const& ) = delete;
    SimpleThreadManager( SimpleThreadManager && ) = delete;

    ~SimpleThreadManager() {
      using namespace std::chrono_literals;

      wait();

      run_thr_ = false;
      spdlog::trace( "gobha::SimpleThreadManager all tasks completed, notifying threads" );
      cv_.notify_all();

      for ( auto &thr : thrs_ ) {
        thr.join();
      }
    }

    auto operator=( SimpleThreadManager const& ) -> SimpleThreadManager& = delete;
    auto operator=( SimpleThreadManager && ) -> SimpleThreadManager& = delete;

    void background( std::function< void() >&& func ) {
      const std::lock_guard lg( q_mut_ );

      bg_funcs_.push_back( std::move( func ) );

      cv_.notify_one();
    }

    void execute( std::function< void() > &&func ) {
      if ( idle_ > 0 ) {
        spdlog::trace( "gobha::SimpleThreadManager There are idle threads" );
        const std::lock_guard lg( q_mut_ );

        fg_funcs_.push_back( std::move( func ) );

        cv_.notify_one();
      } else {
        spdlog::trace( "gobha::SimpleThreadManager There are no idle threads" );
        func();
      }
    }

    void wait() {
      using namespace std::chrono_literals;

      bool finished = false;

      while ( not finished ) {
        std::unique_lock ul( q_mut_ );

        if ( idle_ == thread_limit_ ) {
          if ( cv_.wait_for( ul, 250ms ) == std::cv_status::no_timeout ) {
            finished = bg_funcs_.empty() and fg_funcs_.empty();
          }
        }

        std::this_thread::yield();
      }
    }
  private:
    const size_t                         thread_limit_{std::thread::hardware_concurrency() * 2};
    std::atomic< bool >                  run_thr_{ true };
    std::atomic< size_t >                idle_{ thread_limit_ };
    std::list< std::function< void() > > bg_funcs_;
    std::list< std::function< void() > > fg_funcs_;
    std::vector< std::thread >           thrs_;
    std::mutex                           q_mut_;
    std::condition_variable              cv_;
  };
} // namespace gobha
