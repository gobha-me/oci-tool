#include <functional>
#include <future>
#include <iostream>
#include <random>
#include <thread>
#include <string.h>

std::random_device rd;
std::mt19937 gen( rd() );
std::uniform_int_distribution<> dis( 65, 90 );

char char_gen() {
  return dis( gen );
}

// producer create enough clients for each target digest


// consumer create enough clients for each promice to place data

int main() {
  std::cout << "Hello World" << " " << strlen( "Hello World" ) << std::endl;

  for ( int i = 0; i != 100; i++ )
    std::cout << char_gen() << " ";
  std::cout << std::endl;

  return EXIT_SUCCESS;
}
