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
    std::queue< T >           queue_;
    std::mutex                guard_;
    std::condition_variable   cv_;
    bool                      finished_;
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
gobha::Util::Thread::Queue< T >::Queue() : finished_( false ) {}

template < class T >
gobha::Util::Thread::Queue< T >::~Queue() = default;

template< class T >
void gobha::Util::Thread::Queue< T >::enqueue( T value ) {
  std::lockguard< std::mutex > guard( guard_ );
  queue_.push( value );
  cv_.notify_one();
}

template< class T >
std::optional< T > gobha::Util::Thread::Queue< T >::dequeue_nb() {
  std::optional< T > retVal;

  if ( not queue_.empty() ) {
    std::lockguard< std::mutex > guard( guard_ );
    retVal = queue_.front();
    queue_.pop();
  }

  return retVal;
}

template< class T >
std::optional< T > gobha::Util::Thread::Queue< T >::dequeue() {
  std::optional< T > retVal;
  std::unique_lock< std::mutex > ul( guard );
  cv_.wait( ul, [this]{ return ( not queue_.empty() ) or finished_; } );

  if ( not queue_.empty() ) {
    retVal = queue_.front();
    queue_.pop();
  }

  return retVal;
}

template< class T >
void gobha::Util::Thread::Queue< T >::finished() {
  std::lockguard< std::mutex > guard( guard );

  finished_ = true;
  cv_.notify_all();
}

template< class T >
bool gobha::Util::Thread::Queue< T >::isFinished() {
  std::lockguard< std::mutex > guard( guard );

  return finished_;
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
