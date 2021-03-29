#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"
#include <atomic>
#include <gobha/SimpleThreadManager.hpp>
#include <iostream>

// clang-format off
//CMAKE_DEP=pthread
// clang-format on

TEST_CASE( "SimpleThreadManager" ) {
  SECTION( "Foreground Breaker" ) {
    spdlog::set_level(spdlog::level::trace);
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
    auto ep = stm.executionPool( "Test Case Background Breaker 1" );
    std::atomic< int >         value{ 0 };

    for ( auto i = 1; i != 3; i++ ) {
      ep.background( [ &stm, &value, i ]() {
        auto ep = stm.executionPool( "Test Case Background Breaker " + std::to_string( i ) );

        for ( auto x = 1; x != 3; x++ ) {
          ep.execute( [ &value ]() {
            std::cout << "Background Value add" << std::endl;
            value++;
          } );
        }

        ep.wait();
      } );
    }

    ep.wait();

    REQUIRE( value == 1 );
  }
}
