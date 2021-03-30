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
    class ExecutionPool {
    public:
      ExecutionPool( SimpleThreadManager * stm ) : stm_( stm ) {
        spdlog::trace( "gobha::SimpleThreadManager::ExecutionPool constructing" );
      }
      ExecutionPool( std::string const location, SimpleThreadManager* stm ) : location_( location ), stm_( stm ) {
        spdlog::trace( "gobha::SimpleThreadManager::ExecutionPool constructing {}", location_ );
      }
      ~ExecutionPool() {
        spdlog::trace( "~gobha::SimpleThreadManager::ExecutionPool deconstructing {}", location_ );
        while ( count_ > 0 ) {
          spdlog::trace( "~gobha::SimpleThreadManager::ExecutionPool {} task(s) left", count_ );
          std::this_thread::yield();
        }
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
        stm_->background_( [&func, this]() {
            spdlog::trace( "gobha::SimpleThreadManager::ExecutionPool::background {}", count_ );
            func();
            count_--;
          } );
      }

      void execute( std::function< void() >&& func ) {
        count_++;
        stm_->execute_( [&func, this]() {
            spdlog::trace( "gobha::SimpleThreadManager::ExecutionPool::execute {}", count_ );
            func();
            count_--;
          } );
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
      spdlog::trace( "gobha::SimpleThreadManager Destructor" );
      using namespace std::chrono_literals;
      auto completed = false;

      while ( not completed ) {
        std::unique_lock< std::mutex > ul( fq_mutex_ );

        cv_.wait_for( ul, 250ms );

        completed = f_func_queue_.empty() and b_func_queue_.empty();
      }

      run_thr_ = false;
      spdlog::trace( "gobha::SimpleThreadManager all tasks completed, notifying threads" );
      cv_.notify_all();

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

    auto executionPool( std::string const location ) -> ExecutionPool {
      return ExecutionPool{ location, this };
    }

    friend class executionPool;

    [[deprecated( "Use executionPool instead" )]]
    void execute( std::function< void() >&& func ) {
      execute_( std::move( func ) );
    }

    [[deprecated( "Use executionPool instead" )]]
    void background( std::function< void() >&& func ) {
      background_( std::move( func ) );
    }
  protected:
    void execute_( std::function< void() >&& func ) {
      bool enqueued{ false };

//      if ( queued_ < capacity_ ) {
//        const std::lock_guard fg_lg( fq_mutex_ );
//        ++queued_;
//      
//        enqueued = true;
//        f_func_queue_.push_back( func );
//        cv_.notify_all();
//      }

      if ( not enqueued ) {
        func();
      }
    }

    void background_( std::function< void() >&& func ) {
      std::this_thread::yield();
      const std::lock_guard fg_lg( fq_mutex_ );
      ++queued_;
      
      b_func_queue_.push_back( std::move( func ) );
      cv_.notify_all();
    }

    auto startManager() -> void {
      using namespace std::chrono_literals;

      if ( not started_ ) {
        started_ = true;

        for ( size_t thr_limit = 0; thr_limit != thrs_.capacity(); ++thr_limit ) {
          thrs_.emplace_back( [ this ]() {
              spdlog::trace( "gobha::SimpleThreadManger Thread Starting" );

              while ( run_thr_ ) {
                std::function< void() > func;

                {
                  std::unique_lock< std::mutex > ul( fq_mutex_ );
                  cv_.wait_for( ul, 250ms, [this]() -> bool { return not f_func_queue_.empty() or not b_func_queue_.empty() or not run_thr_; } );

                  if ( ul.owns_lock() ) {
                    if ( not f_func_queue_.empty() ) {
                      spdlog::trace( "gobha::SimpleThreadManger Pulling foreground work" );
                      func = f_func_queue_.front();
                      f_func_queue_.pop_front();
                    } else if ( not b_func_queue_.empty() ) {
                      spdlog::trace( "gobha::SimpleThreadManger Pulling background work" );
                      func = b_func_queue_.front();
                      b_func_queue_.pop_front();
                    }
                  }

                  cv_.notify_one();
                }

                if ( func != nullptr ) {
                  --queued_;
                  func();
                  func = nullptr;
                  spdlog::trace( "gobha::SimpleThreadManger finished a task" );
                }

                std::this_thread::yield();
              }

              spdlog::trace( "gobha::SimpleThreadManger Thread closing" );
          } );
        }
      }
    }
  private:
    bool                                  started_{ false };
    std::atomic< bool >                   run_thr_{ true };
    std::atomic< size_t >                 queued_{ 0 };
    size_t                                capacity_;
    std::vector< std::thread >            thrs_;
    std::list< std::function< void() > > f_func_queue_;
    std::list< std::function< void() > > b_func_queue_;
    std::condition_variable               cv_;
    std::mutex                            aq_mutex_;
    std::mutex                            bq_mutex_;
    std::mutex                            fq_mutex_;
  };
} // namespace gobha
