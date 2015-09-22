#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include "uthread.h"
#include "uthread_mutex_cond.h"

#define NUM_ITERATIONS 10

#ifdef VERBOSE
#define VERBOSE_PRINT(S, ...) printf (S, ##__VA_ARGS__);
#else
#define VERBOSE_PRINT(S, ...) ;
#endif

/**
 * You might find these declarations helpful.
 *   Note that Resource enum had values 1, 2 and 4 so you can combine resources;
 *   e.g., having a MATCH and PAPER is the value MATCH | PAPER == 1 | 2 == 3
 */
enum Resource            {    MATCH = 1, PAPER = 2,   TOBACCO = 4};
char* resource_name [] = {"", "match",   "paper", "", "tobacco"};

int signal_count [5];  // # of times resource signalled
int smoke_count  [5];  // # of times smoker with resource smoked


struct Agent {
  uthread_mutex_t mutex;
  uthread_cond_t  match;
  uthread_cond_t  paper;
  uthread_cond_t  tobacco;
  uthread_cond_t  smoke;
    int m; // number of matches available
    int p; // number of paper available
    int t; // number of tobacco available
    uthread_cond_t paperSmoker;
    uthread_cond_t matchSmoker;
    uthread_cond_t tobaccoSmoker;
};

struct Agent* createAgent() {
  struct Agent* agent = malloc (sizeof (struct Agent));
  agent->mutex   = uthread_mutex_create();
  agent->paper   = uthread_cond_create (agent->mutex);
  agent->match   = uthread_cond_create (agent->mutex);
  agent->tobacco = uthread_cond_create (agent->mutex);
  agent->smoke   = uthread_cond_create (agent->mutex);
    agent->m = 0;
    agent->p = 0;
    agent->t = 0;
    agent->paperSmoker   = uthread_cond_create (agent->mutex);
    agent->matchSmoker  = uthread_cond_create (agent->mutex);
    agent->tobaccoSmoker = uthread_cond_create (agent->mutex);
  return agent;
}

//
// TODO
// You will probably need to add some procedures and struct etc.
//

void* matchHandler(void* av) {
    struct Agent* a = av;
    
    
    VERBOSE_PRINT("try lock 1 \n");
    uthread_mutex_lock (a->mutex);
    VERBOSE_PRINT("grabbed lock 1 \n");
    while (1) {
        
        
        VERBOSE_PRINT("waiting for match 1 \n");
        uthread_cond_wait(a->match); // wait for the match from agent
        VERBOSE_PRINT("awake 1 \n");
        
        // if first call
        a->m++;
        
        // if second call
        if (a->p > 0) {
               VERBOSE_PRINT("sent tobacco signal 1 \n");
            uthread_cond_signal(a->tobaccoSmoker); // if there is paper, signal tobacco smoker
        }
        if (a->t > 0) {
             VERBOSE_PRINT("sent paper signal 1 \n");
            uthread_cond_signal(a->paperSmoker); // if there is tobacco, signal paper smoke
        }
        
    }
    
    uthread_mutex_unlock(a->mutex);
    VERBOSE_PRINT("unlock 1\n");
    return NULL;
}

void* paperHandler(void* av) {
    
    struct Agent* a = av;
    
    VERBOSE_PRINT("try lock 2 \n");
    uthread_mutex_lock (a->mutex);
    VERBOSE_PRINT("grabbed lock 2 \n");
    
    while (1) {
        
        VERBOSE_PRINT("waiting for paper 2\n");
        uthread_cond_wait(a->paper);
        VERBOSE_PRINT("awake 2 \n");
        
        a->p++;
        
        if (a->m > 0) {
            VERBOSE_PRINT("sent tobacco signal 2 \n");
            uthread_cond_signal(a->tobaccoSmoker);
        }
       if (a->t > 0) {
           VERBOSE_PRINT("sent match signal 2 \n");
            uthread_cond_signal(a->matchSmoker);
        }
        
    }
    
    uthread_mutex_unlock(a->mutex);
    VERBOSE_PRINT("unlock 2 \n");
    return NULL;
}

void* tobaccoHandler(void* av) {
    struct Agent* a = av;
    
    VERBOSE_PRINT("try lock 3 \n");
    uthread_mutex_lock (a->mutex);
    VERBOSE_PRINT("grabbed lock 3 \n");
    while (1) {
        
        VERBOSE_PRINT("waiting for tobacco 3\n");
        uthread_cond_wait(a->tobacco);
        VERBOSE_PRINT("awake 3 \n");
        
        a->t++;
        
        if (a->m > 0) {
            VERBOSE_PRINT("signaled paper 3\n");
            uthread_cond_signal(a->paperSmoker);
        }
        else if (a->p > 0) {
            VERBOSE_PRINT("signaled match 3 \n");
            uthread_cond_signal(a->matchSmoker);
        }
        
    }
    
    uthread_mutex_unlock(a->mutex);
    VERBOSE_PRINT("unlock 3 \n");
    return NULL;
}


void* tobacco(void* av) {
    
    struct Agent* a = av;
    
    uthread_mutex_lock (a->mutex);
    while (1) {
        
        uthread_cond_wait(a->tobaccoSmoker);

    smoke_count[4]++;
    a->p--;
    a->m--;
    uthread_cond_signal (a->smoke);
    
    
    }
    
    uthread_mutex_unlock (a->mutex);
    return NULL;
}

void* match(void* av) {
    struct Agent* a = av;
    
    uthread_mutex_lock (a->mutex);
    while (1) {
    uthread_cond_wait(a->matchSmoker);
    
    smoke_count[1]++;
    a->p--;
    a->t--;
    uthread_cond_signal (a->smoke);
    
    }
    
    uthread_mutex_unlock (a->mutex);
    return NULL;
}
    
void* paper(void* av) {
    struct Agent* a = av;
    
    uthread_mutex_lock (a->mutex);
    while (1) {
    uthread_cond_wait(a->paperSmoker);
    
    
    smoke_count[2]++;
    a->t--;
    a->m--;
    
    uthread_cond_signal (a->smoke);
    }
    
    uthread_mutex_unlock (a->mutex);
    return NULL;
    
    }
/**
 * This is the agent procedure.  It is complete and you shouldn't change it in
 * any material way.  You can re-write it if you like, but be sure that all it does
 * is choose 2 random reasources, signal their condition variables, and then wait
 * wait for a smoker to smoke.
 */
void* agent (void* av) {
  struct Agent* a = av;
  static const int choices[]         = {MATCH|PAPER, MATCH|TOBACCO, PAPER|TOBACCO};
  static const int matching_smoker[] = {TOBACCO,     PAPER,         MATCH};
  
  uthread_mutex_lock (a->mutex);
    for (int i = 0; i < NUM_ITERATIONS; i++) {
      int r = random() % 3;   // int r is a random number between 0 and 3
      signal_count [matching_smoker [r]] ++;
      int c = choices [r];  // some choice in terms of ints
      if (c & MATCH) {
        VERBOSE_PRINT ("match available\n");
        uthread_cond_signal (a->match); // signal match if AND contains 1
      }
      if (c & PAPER) {
        VERBOSE_PRINT ("paper available\n"); // signal match if AND contains 2
        uthread_cond_signal (a->paper);
      }
      if (c & TOBACCO) {
        VERBOSE_PRINT ("tobacco available\n"); // signal match if AND contains 4
        uthread_cond_signal (a->tobacco);
      }
      VERBOSE_PRINT ("agent is waiting for smoker to smoke\n"); //wait for smoker to signal
      uthread_cond_wait (a->smoke);
    }
  uthread_mutex_unlock (a->mutex);
  return NULL;
}

int main (int argc, char** argv) { // agents last so that smokers have something first
    uthread_init (1); //multi core will make agent finish running before other threads initiated

  struct Agent*  a = createAgent();

    uthread_create(matchHandler, a);
    uthread_create(paperHandler, a);
    uthread_create(tobaccoHandler, a);
    
    uthread_create(match, a);
    uthread_create(paper, a);
    uthread_create(tobacco, a);
    
    uthread_join (uthread_create(agent, a), 0);
    
  assert (signal_count [MATCH]   == smoke_count [MATCH]);
  assert (signal_count [PAPER]   == smoke_count [PAPER]);
  assert (signal_count [TOBACCO] == smoke_count [TOBACCO]);
  assert (smoke_count [MATCH] + smoke_count [PAPER] + smoke_count [TOBACCO] == NUM_ITERATIONS);
  printf ("Smoke counts: %d matches, %d paper, %d tobacco\n",
          smoke_count [MATCH], smoke_count [PAPER], smoke_count [TOBACCO]);
}