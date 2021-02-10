#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"
#include <gobha/SimpleThreadManager.hpp>

TEST_CASE( "Failure" ) {
  REQUIRE( 1 == 0 );
}
