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
    gobha::SimpleThreadManager stm{ 1 };
    auto ep = stm.executionPool( "Test Case Foreground Breaker 1" );
    std::atomic< int >         value{ 0 };

    ep.execute( [ &stm, &value ]() {
      auto ep = stm.executionPool( "Test Case Foreground Breaker 2" );
      ep.execute( [ &stm, &value ]() {
        auto ep = stm.executionPool( "Test Case Foreground Breaker 3" );
        ep.execute( [ &stm, &value ]() {
          auto ep = stm.executionPool( "Test Case Foreground Breaker 4" );
          ep.execute( [ &value ]() {
            std::cout << "Foreground Value add " << value << std::endl;
            value++;
          } );

          ep.wait();
        } );

        ep.wait();
      } );

      ep.wait();
    } );

    ep.wait();

    REQUIRE( value == 1 );
  }

  SECTION( "Background Breaker" ) {
    gobha::SimpleThreadManager stm{};
    auto ep = stm.executionPool( "Test Case Background Breaker Outter" );
    std::atomic< int >         value{ 0 };

    for ( auto i = 0; i != 8; i++ ) {
      ep.background( [ &stm, &value, i ]() {
        auto ep = stm.executionPool( "Test Case Background Breaker Inner " + std::to_string( i ) );

        ep.execute( [ &value, i ]() {
          std::cout << "Background Value add Inner execution " << i << std::endl;
          value++;
        } );

        ep.wait();
      } );
    }

    ep.wait();

    REQUIRE( value == 1 );
  }
}
