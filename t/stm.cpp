#include <atomic>
#include <list>
#include <mutex>
#include <thread>
#include <spdlog/spdlog.h>

class SimpleThreadManager {
public:
  SimpleThreadManager() {
    using namespace std::chrono_literals;

    for ( size_t thr_limit = 0; thr_limit != thread_limit_; ++thr_limit ) {
      thrs_.emplace_back( [ this ]() {
        spdlog::trace( "Thread Starting" );

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
      } );
    }
  }

  ~SimpleThreadManager() {
    using namespace std::chrono_literals;

    bool finished = false;

    while ( not finished ) {
      std::unique_lock ul( q_mut_ );

      if ( cv_.wait_for( ul, 250ms ) == std::cv_status::no_timeout ) {
        finished = bg_funcs_.empty() and fg_funcs_.empty();
      }

      std::this_thread::yield();
    }

    run_thr_ = false;
    cv_.notify_all();

    for ( auto &thr : thrs_ ) {
      thr.join();
    }
  }

  void background( std::function< void() >&& func ) {
    const std::lock_guard lg( q_mut_ );

    bg_funcs_.push_back( std::move( func ) );

    cv_.notify_one();
  }

  void execute( std::function< void() > &&func ) {
    if ( idle_ > 0 ) {
      spdlog::trace( "There are idle threads" );
      const std::lock_guard lg( q_mut_ );

      fg_funcs_.push_back( std::move( func ) );

      cv_.notify_one();
    } else {
      spdlog::trace( "There are no idle threads" );
      func();
    }
  }

  void wait() {
    using namespace std::chrono_literals;

    bool finished = false;

    while ( not finished ) {
      std::unique_lock ul( q_mut_ );

      if ( cv_.wait_for( ul, 250ms ) == std::cv_status::no_timeout ) {
        finished = bg_funcs_.empty() and fg_funcs_.empty();
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

int main() {
  spdlog::set_level(spdlog::level::trace);
  spdlog::set_pattern( "[%H:%M:%S] [%^%l%$] [thread %t] %v" );

  std::atomic< int > value{ 0 };
  { // anon block so fe goes out of scope and cleans up threads
    SimpleThreadManager stm{};

    for ( std::uint16_t i = 0; i != 8; i++ ) {
      stm.background( [ i, &value, &stm ]() {
        stm.execute( [ i, &value ] {
          spdlog::info( "Execution {}", i);
          value++;
        } );
      } );
    }

    stm.wait();
  }

  spdlog::info( "Value = {}", value );
}
