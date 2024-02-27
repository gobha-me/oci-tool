#include <catch2/catch_all.hpp>
#include <gobha/SimpleThreadManager.hpp>

TEST_CASE( "DelayedCall" ) {
  bool tested = false;

  // each section starts with fresh values as defined above
  SECTION( "Create DelayedCall Object" ) {
    { // DelayedCall, executes lamdba on descruction
      gobha::DelayedCall test( [ &tested ]() { tested = true; } );

      REQUIRE( tested == false );
    }

    REQUIRE( tested == true );
  }
}
