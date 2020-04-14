#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <random>
#include <thread>
#include <string.h>
#include <gobha/Util/Thread/Queue.hpp>

std::random_device rd;
std::mt19937 gen( rd() );
std::uniform_int_distribution<> char_dis( 65, 90 );
std::uniform_int_distribution< std::uint32_t > size_dis( 0, std::numeric_limits< std::uint32_t >::max() );

char char_gen() {
  return char_dis( gen );
}

uint32_t size_gen() {
  return size_dis( gen );
}

using Buffer   = std::pair< uint64_t, std::array< char, 256 > >;
using BufferQ  = gobha::Util::Thread::Queue< Buffer >;
using ManifestMap = std::map< std::string, BufferQ >; // Digest -> Buffer

void producer( BufferQ& byte_buffer, std::uint32_t gen_size ) {
  Buffer t_buff;
  auto max_buff_size = t_buff.second.max_size();

  std::cout << "Gen Size: " << gen_size << " Max Buff Size: " << max_buff_size << std::endl;

  std::uint32_t size = 0;
  for (; size != gen_size; size ++ ) {
    auto index = size % max_buff_size;
    if ( index == 0 )
      t_buff = Buffer();

    t_buff.second[ index ] = char_gen();

    if ( index == max_buff_size - 1 ) {
      t_buff.first = max_buff_size;
      byte_buffer.enqueue( t_buff );
    }
  }

  if ( ( size % max_buff_size ) < max_buff_size ) {
    t_buff.first = ( size % max_buff_size );
    byte_buffer.enqueue( t_buff );
  }

  byte_buffer.finished();

}

void receiver( BufferQ& byte_buffer, std::uint32_t gen_size ) {
  std::uint32_t received = 0;

  while( not byte_buffer.isFinished() ) {
    auto value = byte_buffer.dequeue();
    if ( value.has_value() ) {
      received += value->first;
    }
  }

  auto value = byte_buffer.dequeue();
  if ( value.has_value() ) {
    received += value->first;
  }

  if ( gen_size != received ) {
    std::cout << "Receive failed! Got: " << received << " Expected: " << gen_size << std::endl;
  }
}

// producer create enough clients for each target digest


// consumer create enough clients for each promice to place data

int main() {
  std::cout << "Hello World" << " " << strlen( "Hello World" ) << std::endl;

  auto max_size = size_gen();
  BufferQ buffer_q;

  std::thread t1( producer, std::ref( buffer_q ), max_size );
  std::thread t2( receiver, std::ref( buffer_q ), max_size );

  t1.join();
  t2.join();

  return EXIT_SUCCESS;
}
