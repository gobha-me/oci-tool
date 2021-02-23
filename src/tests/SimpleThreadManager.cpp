#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"
#include <atomic>
#include <gobha/SimpleThreadManager.hpp>
#include <iostream>

// CMAKE_DEP=pthread

TEST_CASE( "SimpleThreadManager" ) {
  SECTION( "Forground Breaker" ) {
    gobha::SimpleThreadManager stm{ 1 };
    std::atomic< int >         value{ 0 };

    stm.execute( [ &stm, &value ]() {
      stm.execute( [ &stm, &value ]() {
        stm.execute( [ &stm, &value ]() {
          stm.execute( [ &stm, &value ]() {
            std::cout << "Executing Value add" << std::endl;
            value++;
          } );
        } );
      } );
    } );

    while ( value == 0 ) {
    }

    REQUIRE( value == 1 );
  }

  SECTION( "Background Breaker" ) {
    gobha::SimpleThreadManager stm{ 1 };
    std::atomic< int >         value{ 0 };

    stm.background( [ &stm, &value ]() {
      stm.background( [ &stm, &value ]() {
        stm.background( [ &stm, &value ]() {
          stm.background( [&value ]() {
            std::cout << "Executing Value add" << std::endl;
            value++;
          } );
        } );
      } );
    } );

    while ( value == 0 ) {
    }

    REQUIRE( value == 1 );
  }
}
