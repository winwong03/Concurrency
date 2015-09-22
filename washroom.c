#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include "uthread.h"
#include "uthread_mutex_cond.h"

#ifdef VERBOSE
#define VERBOSE_PRINT(S, ...) printf (S, ##__VA_ARGS__);
#else
#define VERBOSE_PRINT(S, ...) ;
#endif

#define MAX_OCCUPANCY      3
#define NUM_ITERATIONS     300
#define NUM_PEOPLE         50
#define FAIR_WAITING_COUNT 4

/**
 * You might find these declarations useful.
 */
enum Sex {MALE = 0, FEMALE = 1};
const static enum Sex otherSex [] = {FEMALE, MALE};

struct Washroom {
    uthread_mutex_t mutex;
    uthread_cond_t femaleQueue;
    uthread_cond_t maleQueue;
    int numInside;
    int sex;
    int startTime;
    int endTime;
    int waiting [2];

};

struct Washroom* createWashroom() {
  struct Washroom* washroom = malloc (sizeof (struct Washroom));
    washroom->mutex = uthread_mutex_create();
    washroom->femaleQueue = uthread_cond_create(washroom->mutex);
    washroom->maleQueue = uthread_cond_create(washroom->mutex);
    washroom->numInside = 0; // set number
    washroom->sex = 0; //sex currently in washroom
    washroom->startTime = 0;
    washroom->endTime = 0;
    washroom->waiting[0] = 0;
    washroom->waiting[1] = 0;
    
  return washroom;
}

#define WAITING_HISTOGRAM_SIZE (NUM_ITERATIONS * NUM_PEOPLE)
int             entryTicker;  // incremented with each entry
int             waitingHistogram         [WAITING_HISTOGRAM_SIZE];
int             waitingHistogramOverflow;
uthread_mutex_t waitingHistogrammutex;
int             occupancyHistogram       [2] [MAX_OCCUPANCY + 1];

void recordWaitingTime (int waitingTime) {
    uthread_mutex_lock (waitingHistogrammutex);
    if (waitingTime < WAITING_HISTOGRAM_SIZE)
        waitingHistogram [waitingTime]++;
    else
        waitingHistogramOverflow ++;
    uthread_mutex_unlock (waitingHistogrammutex);
}

void enterWashroom (struct Washroom* washroom, enum Sex sex) {
    uthread_mutex_lock(washroom->mutex);
    
    while (washroom->sex != sex || washroom->numInside >= MAX_OCCUPANCY) {
            if (sex == 1){
                washroom->waiting[1]++;
                uthread_cond_wait (washroom->femaleQueue); //washroom->femaleQ
            }
            else {
                washroom->waiting[0]++;
                uthread_cond_wait (washroom->maleQueue); //washroom->maleQ
            }
        }

    washroom->numInside++;
    if (washroom->waiting[sex] != 0) {washroom->waiting[sex]--;}
    occupancyHistogram[washroom->sex][washroom->numInside]++;
      entryTicker++;
    
    uthread_mutex_unlock(washroom->mutex);
}

void leaveWashroom (struct Washroom* washroom) { //case where more than 3 sex but other don't have gender will change anyway and deadlock.
 
    uthread_mutex_lock(washroom->mutex);
    
    washroom->numInside--;
    
    if (washroom->numInside == 0 && washroom->waiting[otherSex[washroom->sex]] > 0) {
        washroom->sex = otherSex[washroom->sex];
    
        if (washroom->sex == 0) {
            for (int i = 0; i < MAX_OCCUPANCY; i++) {
            uthread_cond_signal (washroom->maleQueue);
            }
        } else {
            for (int j = 0; j < MAX_OCCUPANCY; j++) {
                uthread_cond_signal (washroom->femaleQueue);
            }
        }
    } else {
        
        if (washroom->sex == 0) {
            for (int i = 0; i < MAX_OCCUPANCY; i++) {
                uthread_cond_signal (washroom->maleQueue);
            }
        } else {
            for (int i = 0; i < MAX_OCCUPANCY; i++) {
                uthread_cond_signal (washroom->femaleQueue);
            }
        }
    }
    
    uthread_mutex_unlock(washroom->mutex);
}

void* femaleEmployee (void* w) {
    struct Washroom* washroom = (struct Washroom*) w;
    
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        washroom->startTime = entryTicker++;
        enterWashroom(washroom, 1);
        washroom->endTime = entryTicker; //just added now
        for (int j = 0; j < NUM_PEOPLE; j++) {
            uthread_yield();
        }
        
        leaveWashroom(washroom);
        
        for (int k = 0; k < NUM_PEOPLE; k++) {
            uthread_yield();
        }
        
        int waitingTime = washroom->endTime - washroom->startTime;
        recordWaitingTime(waitingTime);
    }
    return NULL;
}

void* maleEmployee (void* w) {
    struct Washroom* washroom = (struct Washroom*) w;
    
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        washroom->startTime = entryTicker;
        enterWashroom(washroom, 0);
        
        for (int j = 0; j < NUM_PEOPLE; j++) {
            uthread_yield();
        }
        
        leaveWashroom(washroom);
        
        for (int k = 0; k < NUM_PEOPLE; k++) {
            uthread_yield();
        }
        
        int waitingTime = washroom->endTime - washroom->startTime;
        recordWaitingTime(waitingTime);
    }
    return NULL;
}

//
// TODO
// You will probably need to create some additional produres etc.
//

int main (int argc, char** argv) {
  uthread_init (1);
  struct Washroom* washroom = createWashroom();
  uthread_t        pt [NUM_PEOPLE];
  waitingHistogrammutex = uthread_mutex_create ();
    waitingHistogramOverflow = 0;
    
    for (int i=0; i < NUM_PEOPLE; i+=2) {
        pt[i] = uthread_create (femaleEmployee, washroom);
    }
    for (int m = 1; m < NUM_PEOPLE; m+=2) {
        pt[m] = uthread_create (maleEmployee, washroom);
    }
    for (int j=0; j < NUM_PEOPLE; j++) {
        uthread_join(pt[j], 0);
    }
  
  printf ("Times with 1 male    %d\n", occupancyHistogram [MALE]   [1]);
  printf ("Times with 2 males   %d\n", occupancyHistogram [MALE]   [2]);
  printf ("Times with 3 males   %d\n", occupancyHistogram [MALE]   [3]);
  printf ("Times with 1 female  %d\n", occupancyHistogram [FEMALE] [1]);
  printf ("Times with 2 females %d\n", occupancyHistogram [FEMALE] [2]);
  printf ("Times with 3 females %d\n", occupancyHistogram [FEMALE] [3]);
  printf ("Waiting Histogram\n");
  for (int i=0; i<WAITING_HISTOGRAM_SIZE; i++)
    if (waitingHistogram [i])
      printf ("  Number of times people waited for %d %s to enter: %d\n", i, i==1?"person":"people", waitingHistogram [i]);
  if (waitingHistogramOverflow)
    printf ("  Number of times people waited more than %d entries: %d\n", WAITING_HISTOGRAM_SIZE, waitingHistogramOverflow);
}