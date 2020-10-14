#pragma once
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <list>
#include <mutex>
#include <thread>
#include <vector>

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
    SimpleThreadManager() : SimpleThreadManager( std::thread::hardware_concurrency() * 2 ) {}

    explicit SimpleThreadManager( size_t thr_count ) : capacity_( thr_count ) {
      thrs_.reserve( capacity_ );

      startManager();
    }

    SimpleThreadManager( SimpleThreadManager const& ) = delete;
    SimpleThreadManager( SimpleThreadManager && other ) noexcept {
      capacity_         = other.capacity_;
      f_func_queue_     = std::move( other.f_func_queue_ );
      b_func_queue_     = std::move( other.b_func_queue_ );
      thrs_             = std::move( other.thrs_ );
    }

    ~SimpleThreadManager() {
      using namespace std::chrono_literals;
      auto completed = false;

      while ( not completed ) {
        std::unique_lock< std::mutex > ul( fq_mutex_ );

        cv_.wait_for( ul, 250ms );

        completed = f_func_queue_.empty() and b_func_queue_.empty();
      }

      run_thr_ = false;

      if ( started_ ) {
        for ( auto &thr : thrs_ ) {
          thr.join();
        }
      }
    }

    auto operator=( SimpleThreadManager const& ) -> SimpleThreadManager& = delete;
    auto operator=( SimpleThreadManager && ) -> SimpleThreadManager& = delete;

    auto execute( std::function< void() >&& func ) -> void {
      bool enqueued{ false };

      {
        std::lock_guard< std::mutex > fg_lg( fq_mutex_ );
        
        if ( running_count_ < capacity_ ) {
          enqueued = true;
          f_func_queue_.push_back( func );
          cv_.notify_all();
        }
      }

      if ( not enqueued ) {
        func();
      }
    }

    auto background( std::function< void() >&& func ) -> void {
      std::this_thread::yield();
      std::lock_guard< std::mutex > fg_lg( fq_mutex_ );
      
      b_func_queue_.push_back( func );
      cv_.notify_all();
    }
  protected:
    auto startManager() -> void {
      using namespace std::chrono_literals;

      if ( not started_ ) {
        started_ = true;

        for ( size_t thr_limit = 0; thr_limit != thrs_.capacity(); ++thr_limit ) {
          thrs_.emplace_back( [ this ]() {
              while ( run_thr_ ) {
                std::function< void() > func;
                bool has_func = false;

                {
                  std::unique_lock< std::mutex > ul( fq_mutex_ );
                  cv_.wait_for( ul, 250ms, [this]() -> bool { return not f_func_queue_.empty() or not b_func_queue_.empty(); } );

                  if ( not f_func_queue_.empty() ) {
                    has_func = true;
                    func = f_func_queue_.front();
                    f_func_queue_.pop_front();
                  } else if ( not b_func_queue_.empty() ) {
                    has_func = true;
                    func = b_func_queue_.front();
                    b_func_queue_.pop_front();
                  }
                }

                if ( has_func ) {
                  ++running_count_;
                  func();
                  --running_count_;
                }

                std::this_thread::yield();
              }
          } );
        }
      }
    }
  private:
    bool                                 started_{ false };
    std::atomic< bool >                  run_thr_{ true };
    std::atomic< size_t >                running_count_{ 0 };
    size_t                               capacity_;
    std::vector< std::thread >           thrs_;
    std::list< std::function< void() > > f_func_queue_;
    std::list< std::function< void() > > b_func_queue_;
    std::condition_variable              cv_;
    std::mutex                           fq_mutex_;
  };
} // namespace gobha
