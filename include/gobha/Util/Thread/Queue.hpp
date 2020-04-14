/*
 * =====================================================================================
 *
 *       Filename:  Queue.hpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  02/03/2020 08:50:18 AM
 *       Revision:  none
 *       Compiler:  clang++
 *
 *         Author:  Jeffrey Smith (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

// For the moment two types of queues
// Queue          - std::mutex protected, simple enough of an implementation
// ScheduledQueue - Saw a video on this, seems uint16_teresting, want to experiment
namespace gobha::Util::Thread {
  template < class T >
  class Queue;

  template < class T, uint16_t SIZE = 6 >
  class ScheduledQueue;
} // namespace gobha::Util::Thread

namespace gobha::Util::Thread {
  template < class T >
  class Queue {
  public:
    Queue();
    virtual ~Queue();

    void enqueue( T );
    std::optional< T > dequeue_nb();

    std::optional< T > dequeue();

    void finished();

    bool isFinished();

  protected:
  private:
    std::queue< T >           _queue;
    std::mutex                _guard;
    std::condition_variable   _cv;
    bool                      _finished;
  };


  //  Juggling Razor blades by Herd Sutter - Lock free programming
  template < class T, uint16_t SIZE >
  class ScheduledQueue { // Watched some videos on this 2ish yrs ago -- damn need to research again
  public:
    ScheduledQueue();
    virtual ~ScheduledQueue();

    void enqueue( T );
    std::optional< T > dequeue_nb();

  protected:
  private:
    struct SList;

    std::atomic< SList * > head = nullptr;
  };
} // namespace gobha::Util::Thread


// Implementations
// QUEUE
template < class T >
gobha::Util::Thread::Queue< T >::Queue() : _finished( false ) {}

template < class T >
gobha::Util::Thread::Queue< T >::~Queue() = default;

template< class T >
void gobha::Util::Thread::Queue< T >::enqueue( T value ) {
  std::lock_guard< std::mutex > guard( _guard );
  _queue.push( value );
  _cv.notify_one();
}

template< class T >
std::optional< T > gobha::Util::Thread::Queue< T >::dequeue_nb() {
  std::optional< T > retVal;

  if ( not _queue.empty() ) {
    std::lock_guard< std::mutex > guard( _guard );
    retVal = _queue.front();
    _queue.pop();
  }

  return retVal;
}

template< class T >
std::optional< T > gobha::Util::Thread::Queue< T >::dequeue() {
  std::optional< T > retVal;
  std::unique_lock< std::mutex > ul( _guard );
  _cv.wait( ul, [this]{ return ( not _queue.empty() ) or _finished; } );

  if ( not _queue.empty() ) {
    retVal = _queue.front();
    _queue.pop();
  }

  return retVal;
}

template< class T >
void gobha::Util::Thread::Queue< T >::finished() {
  std::lock_guard< std::mutex > guard( _guard );

  _finished = true;
  _cv.notify_all();
}

template< class T >
bool gobha::Util::Thread::Queue< T >::isFinished() {
  std::lock_guard< std::mutex > guard( _guard );

  return _finished;
}


// SCHEDULED QUEUE
template < class T, uint16_t SIZE >
gobha::Util::Thread::ScheduledQueue< T, SIZE >::ScheduledQueue() {
  head              = new SList;
  SList * orig_head = head;
  for ( uint16_t limit = 0; limit != SIZE; limit++ ) {
    head->node = new SList;
    head = head->node;
  }
  head->node = orig_head;
}

template < class T, uint16_t SIZE >
gobha::Util::Thread::ScheduledQueue< T, SIZE >::~ScheduledQueue() {}

template< class T, uint16_t SIZE >
struct gobha::Util::Thread::ScheduledQueue< T, SIZE >::SList {
  std::atomic< SList * > node;
  std::atomic< T > value;
};
