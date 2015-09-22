#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "uthread.h"
#include "uthread_sem.h"

const int MAX_ITEMS = 10;
const int NUM_ITERATIONS = 200;
const int NUM_CONSUMERS  = 2;
const int NUM_PRODUCERS  = 2;

int producer_wait_count;     // # of times producer had to wait
int consumer_wait_count;     // # of times consumer had to wait
int histogram [MAX_ITEMS+1]; // histogram [i] == # of times list stored i items


// wait = unlock, q, block, wake, lock

struct Pool {
    uthread_sem_t produce;
    uthread_sem_t consume;
    uthread_sem_t mutex;
    int items;
};

struct Pool* createPool() {
    struct Pool* pool = malloc (sizeof (struct Pool));
    pool->items     = 0;
    pool->produce = uthread_sem_create(0); // value is how many threads can run at the same time
    pool->consume = uthread_sem_create(10);
    pool->mutex = uthread_sem_create(1);
  return pool;
}

void* producer (void* pv) {
  struct Pool* p = pv;
    
    for (int i=0; i < NUM_ITERATIONS; i++) {
    
        uthread_sem_wait(p->consume ); //wait for 
        
        uthread_sem_wait(p->mutex);
        assert (p->items < MAX_ITEMS);
        p->items++;
        uthread_sem_signal(p->mutex);
        
        histogram [p->items] += 1;
        uthread_sem_signal(p->produce);
        
    }
    
  return NULL;
}

void* consumer (void* pv) {
  struct Pool* p = pv;
    
    for (int i=0; i<NUM_ITERATIONS; i++) {
        
        uthread_sem_wait(p->produce);
        uthread_sem_wait(p->mutex);
        assert(p->items > 0);
        p->items--;
        assert (p->items >=0);
        uthread_sem_signal(p->mutex);
                    
        histogram [p->items] += 1;
        uthread_sem_signal(p->consume);
    }

  return NULL;
}

int main (int argc, char** argv) {
  uthread_t t[4];

  uthread_init (4);
  struct Pool* p = createPool();
  
    for (int i= 0; i < NUM_PRODUCERS; i++) { //make thread 0 and 1
        t[i] = uthread_create(producer, p);
    }
    
    for (int j = NUM_PRODUCERS; j < NUM_PRODUCERS + NUM_CONSUMERS; j++) {
        t[j] = uthread_create(consumer, p); // make thread 2 and 3
    }
    
    for (int k = 0; k < NUM_CONSUMERS + NUM_PRODUCERS; k++) {
        uthread_join (t[k], 0); //join the threads
    }
    
  printf ("producer_wait_count=%d, consumer_wait_count=%d\n", producer_wait_count, consumer_wait_count);
  printf ("items value histogram:\n");
  int sum=0;
  for (int i = 0; i <= MAX_ITEMS; i++) {
    printf ("  items=%d, %d times\n", i, histogram [i]);
    sum += histogram [i];
  }
  assert (sum == sizeof (t) / sizeof (uthread_t) * NUM_ITERATIONS);
}