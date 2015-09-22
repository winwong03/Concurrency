#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "spinlock.h"
#include "uthread.h"

#define MAX_ITEMS 10

int items = 0;
spinlock_t spinlock;
spinlock_t consumerwait;
spinlock_t producerwait;

const int NUM_ITERATIONS = 1000;
const int NUM_CONSUMERS  = 2;
const int NUM_PRODUCERS  = 2;

int producer_wait_count;     // # of times producer had to wait
int consumer_wait_count;     // # of times consumer had to wait
int histogram [MAX_ITEMS+1]; // histogram [i] == # of times list stored i items

void produce() {
    
    while (1) {
        
        while (items == MAX_ITEMS) { // while max items, wait
            spinlock_lock (&producerwait);
            producer_wait_count++;
            spinlock_unlock (&producerwait);
        }
        
        spinlock_lock (&spinlock);
        
        if (items < MAX_ITEMS) { // if < Max, add items
            items++;
            histogram [items] += 1;
            spinlock_unlock (&spinlock);
            break;
        }
    
       spinlock_unlock (&spinlock);
    }
}

void consume() {
  // TODO ensure proper synchronization
    
    while (1) {
        
        while (items == 0) {
            spinlock_lock(&consumerwait);
            consumer_wait_count++;
            spinlock_unlock(&consumerwait);
        }
    
        spinlock_lock (&spinlock);
    
        if (items > 0) {
            items--;
            histogram[items] += 1;
            spinlock_unlock (&spinlock);
            break;
        }
        
        spinlock_unlock (&spinlock);
        
    }
}

void* producer(void* n) {
    
    for (int i=0; i < NUM_ITERATIONS; i++)
    produce();
    
    return NULL;
    
}

void* consumer(void* n) {
    
    for (int i=0; i< NUM_ITERATIONS; i++)
    consume();

    return NULL;
}

int main (int argc, char** argv) {
    uthread_init (4);
    uthread_t thread [4];
    
    spinlock_create(&spinlock);
    spinlock_create(&producerwait);
    spinlock_create(&consumerwait);
    
    for (int i= 0; i < NUM_PRODUCERS; i++) { //make thread 0 and 1
        thread[i] = uthread_create(producer, 0);
    }
    
    for (int j = NUM_PRODUCERS; j < NUM_PRODUCERS + NUM_CONSUMERS; j++) {
        thread[j] = uthread_create(consumer, 0); // make thread 2 and 3
    }
    
    for (int k = 0; k < NUM_CONSUMERS + NUM_PRODUCERS; k++) {
        uthread_join (thread[k], 0); //join the threads
    }
    
  // TODO create threads to run the producers and consumers
    
  printf("Producer wait: %d\nConsumer wait: %d\n",
         producer_wait_count, consumer_wait_count);
    
  for(int i=0;i<MAX_ITEMS+1;i++)
    printf("items %d count %d\n", i, histogram[i]);
}