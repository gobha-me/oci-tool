#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"
#include <atomic>
#include <gobha/SimpleThreadManager.hpp>
#include <iostream>

// clang-format off
//CMAKE_DEP=pthread
// clang-format on

TEST_CASE( "SimpleThreadManager" ) {
  spdlog::set_level(spdlog::level::trace);
  spdlog::set_pattern( "[%H:%M:%S] [%^%l%$] [thread %t] %v" );

  SECTION( "Foreground Breaker" ) {
    gobha::SimpleThreadManager stm{};
    std::atomic< int >         value{ 0 };

    for ( std::uint16_t i = 0; i != 16; i++ ) {
      stm.execute( [ &stm, &value, i ]() {
        spdlog::info( "Outer Execute {}", i );

        stm.execute( [ &value, i ]() {
          spdlog::info( "Inner Execute {}", i );
          value++;
        } );
      } );
    }

    stm.wait();

    REQUIRE( value == 16 );
  }

  SECTION( "Background Breaker" ) {
    gobha::SimpleThreadManager stm{};
    std::atomic< int >         value{ 0 };

    for ( std::uint16_t i = 0; i != 16; i++ ) {
      stm.background( [ &stm, &value, i ]() {
        spdlog::info( "Background {}", i );

        stm.execute( [ &value, i ]() {
          spdlog::info( "Execute {}", i );
          value++;
        } );
      } );
    }

    stm.wait();

    REQUIRE( value == 16 );
  }
}
