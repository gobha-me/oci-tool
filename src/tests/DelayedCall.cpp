#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"
#include <gobha/SimpleThreadManager.hpp>

unsigned int Factorial( unsigned int number ) {
    return number <= 1 ? number : Factorial(number-1)*number;
}

TEST_CASE( "DelayedCall" ) {
  bool tested = false;

  // each section starts with fresh values as defined above
  SECTION( "Create DelayedCall Object" ) {
    { // DelayedCall, executes lamdba on descruction
      gobha::DelayedCall test([&tested]() { tested = true; } );

      REQUIRE( tested == false );
    }

    REQUIRE( tested == true );
  }
}
