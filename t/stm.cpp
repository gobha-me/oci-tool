#include <atomic>
#include <list>
#include <mutex>
#include <thread>
#include <spdlog/spdlog.h>

class funcExec {
public:
  funcExec() {
    using namespace std::chrono_literals;

    for ( size_t thr_limit = 0; thr_limit != std::thread::hardware_concurrency() * 2; ++thr_limit ) {
      thrs_.emplace_back( [ this ]() {
          spdlog::trace( "Thread Starting" );

          while ( run_thr_ ) {
            std::function< void() > func;

            {
              std::unique_lock< std::mutex > ul( q_mut_ );
              cv_.wait_for( ul, 250ms, [this]() -> bool { return not funcs_.empty(); } );

              if ( ul.owns_lock() and not funcs_.empty() ) {
                func = funcs_.front();

                funcs_.pop_front();
              }
            }

            if ( func != nullptr ) {
              func();
            }
          }
        } );
    }
  }

  ~funcExec() {
    while ( not funcs_.empty() ) {}

    run_thr_ = false;

    for ( auto &thr : thrs_ ) {
      thr.join();
    }
  }

  void enqueue( std::function< void() >&& func ) {
    const std::lock_guard lg( q_mut_ );

    funcs_.push_back( std::move( func ) );

    cv_.notify_one();
  }
private:
  std::atomic< bool >                  run_thr_{ true };
  std::list< std::function< void() > > funcs_;
  std::vector< std::thread >           thrs_;
  std::mutex                           q_mut_;
  std::condition_variable              cv_;
};

int main() {
  spdlog::set_level(spdlog::level::trace);
  spdlog::set_pattern( "[%H:%M:%S] [%^%l%$] [thread %t] %v" );

  std::atomic< int > value{ 0 };
  {
    funcExec fe{};

    for ( std::uint16_t i = 0; i != 8; i++ ) {
      fe.enqueue([ i, &value ]() {
          spdlog::info( "Execution {}", i);
          value++;
        });
    }
  }

  spdlog::info( "Value = {}", value );
}
