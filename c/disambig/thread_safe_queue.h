#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H
#include <queue>
#include <pthread.h>
#include <exception>

using namespace std;

class QueueEmptyException : public std::exception
{
  virtual const char* what() const throw()
  {
    return "Queue Empty";
  }
};

template <typename T>
class ThreadSafeQueue {
  queue<T> data;
  unsigned int capacity;
  pthread_mutex_t mutex;
	pthread_cond_t cond_full;
	pthread_cond_t cond_empty;

  public:
    ThreadSafeQueue(unsigned int capacity) {
      this->capacity = capacity;
      pthread_mutex_init(&mutex,NULL);
      pthread_cond_init(&cond_full,NULL);
      pthread_cond_init(&cond_empty,NULL);
    };

    ThreadSafeQueue() {
      ThreadSafeQueue(0);
    }

    void enqueue(T const &);
    unsigned int size();
    T dequeue();
};

#include "thread_safe_queue.cpp"
#endif 
