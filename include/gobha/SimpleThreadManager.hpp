#pragma once
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <mutex>
#include <thread>
#include <vector>
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
    class ExecutionPool {
    public:
      ExecutionPool( SimpleThreadManager * stm ) : stm_( stm ) {
        spdlog::trace( "gobha::SimpleThreadManger::ExecutionPool constructing" );
      }
      ExecutionPool( std::string const & location, SimpleThreadManager* stm ) : location_( location ), stm_( stm ) {
        spdlog::trace( "gobha::SimpleThreadManger::ExecutionPool constructing {}", location_ );
      }
      ~ExecutionPool() {
        spdlog::trace( "gobha::SimpleThreadManger::ExecutionPool deconstructing {}", location_ );
      }

      ExecutionPool( ExecutionPool const & ) = delete;
      ExecutionPool( ExecutionPool && ) = delete;

      auto operator=( ExecutionPool const& ) -> ExecutionPool& = delete;
      auto operator=( ExecutionPool && ) -> ExecutionPool& = delete;

      operator bool() {
        if ( count_ > 0 )
          return true;
        return false;
      }

      void background( std::function< void() >&& func ) {
        count_++;
        stm_->background( [&func, this]() {
            func();
            count_--;
          } );
      }

      void execute( std::function< void() >&& func ) {
        count_++;
        stm_->execute( [&func, this]() {
            func();
            count_--;
          } );
      }

      void wait() {
        while ( count_ > 0 ) {
          std::this_thread::yield();
        }
      }
    protected:
    private:
      std::string                location_;
      std::atomic< std::size_t > count_{0};
      SimpleThreadManager*       stm_{nullptr};
    };

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
      spdlog::trace( "gobha::SimpleThreadManger Destructor" );
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

    auto executionPool() -> ExecutionPool {
      return ExecutionPool{ this };
    }

    auto executionPool( std::string const & location ) -> ExecutionPool {
      return ExecutionPool{ location, this };
    }

    friend class executionPool;

    [[deprecated( "Use executionPool instead" )]]
    auto execute( std::function< void() >&& func ) -> void {
      bool enqueued{ false };

      if ( running_count_ < capacity_ ) {
        const std::lock_guard fg_lg( fq_mutex_ );
      
        enqueued = true;
        f_func_queue_.emplace( func );
      }

      if ( not enqueued ) {
        func();
      }
    }

    [[deprecated( "Use executionPool instead" )]]
    auto background( std::function< void() >&& func ) -> void {
      std::this_thread::yield();
      const std::lock_guard fg_lg( bq_mutex_ );
      
      b_func_queue_.emplace( func );
    }
  protected:
    auto startManager() -> void {
      using namespace std::chrono_literals;

      if ( not started_ ) {
        started_ = true;

        for ( size_t thr_limit = 0; thr_limit != thrs_.capacity(); ++thr_limit ) {
          thrs_.emplace_back( [ this ]() {
              spdlog::trace( "gobha::SimpleThreadManger Thread Starting" );

              while ( run_thr_ ) {
                std::function< void() > func;

                if ( not f_func_queue_.empty() ) {
                  const std::lock_guard fg_lg( fq_mutex_ );

                  if ( not f_func_queue_.empty() ) {
                    spdlog::trace( "gobha::SimpleThreadManger Pulling foreground work" );
                    func = f_func_queue_.front();
                    f_func_queue_.pop();
                  }
                } else if ( not b_func_queue_.empty() ) {
                  const std::lock_guard bg_lg( bq_mutex_ );

                  if ( not b_func_queue_.empty() ) {
                    spdlog::trace( "gobha::SimpleThreadManger Pulling background work" );
                    func = b_func_queue_.front();
                    b_func_queue_.pop();
                  }
                }

                if ( func != nullptr ) {
                  ++running_count_;
                  func();
                  spdlog::trace( "gobha::SimpleThreadManger finished a task" );
                  --running_count_;
                }

                std::this_thread::yield();
              }

              spdlog::trace( "gobha::SimpleThreadManger Thread closing" );
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
    std::queue< std::function< void() > > f_func_queue_;
    std::queue< std::function< void() > > b_func_queue_;
    std::condition_variable              cv_;
    std::mutex                           aq_mutex_;
    std::mutex                           bq_mutex_;
    std::mutex                           fq_mutex_;
  };
} // namespace gobha
