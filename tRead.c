#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/errno.h>
#include <assert.h>
#include "queue.h"
#include "disk.h"
#include "uthread.h"

int sum = 0;

queue_t prq;

void interruptServiceRoutine () { //called from disk_scheduleRead
 
    uthread_t thread = queue_dequeue (&prq);
    uthread_unblock (thread);
   
    
  // TODO
}

void blockUntilComplete() {
    
    queue_enqueue (&prq, uthread_self());
    uthread_block();
    
  // TODO
}

void handleRead (char* buf, int nbytes, int blockno) {
  
  assert (*((int*)buf) == blockno);
    
  sum += *(((int*) buf) + 1);
}

/**
 * Struct provided as argument to readAndHandleBlock
 */
struct readAndHandleBlockArgs {
  char* buf;
  int   nbytes;
  int   blockno;
};

void* readAndHandleBlock (void* args_voidstar) { // takes in a struct as argument
    
    struct readAndHandleBlockArgs* rhb = (struct readAndHandleBlockArgs*) args_voidstar;
    
    disk_scheduleRead(rhb->buf, rhb->nbytes, rhb->blockno);
   
    blockUntilComplete(); // stop thread from accessing disk

    handleRead(rhb->buf, rhb->nbytes, rhb->blockno); // sum
    
    free(rhb->buf);
    free(rhb);

    
  // TODO Synchronously:
  //   (1) call disk_scheduleRead to request the block
  //   (2) wait read for it to complete
  //   (3) call handleRead
  return NULL;
}

void run (int numBlocks) {
  uthread_t thread [numBlocks];
    
    
  for (int blockno = 0; blockno < numBlocks; blockno++) {
      // if readandhandleblock is called then it needs a struct as argument. create struct
      
      char* buffer = malloc (1000); //need to dynamically allocate each time or it will overwrite
      
      struct readAndHandleBlockArgs* rhblock = malloc (sizeof (struct readAndHandleBlockArgs));
      
      rhblock->buf = buffer;
    
      rhblock->nbytes = 1000;
      
      rhblock->blockno = blockno;
      
      thread[blockno] =  uthread_create(readAndHandleBlock, rhblock);
      
      //free(buffer);
      
      
     // free(readandhandleblockarg);
      
    // TODO
    // call readAndHandleBlock in a way that allows this
    // operation to be synchronous without stalling the CPU
  }
    
  for (int i=0; i<numBlocks; i++)
    uthread_join (thread [i], 0); //will return after all disks have been read
    
    
    
    
}

int main (int argc, char** argv) {
  static const char* usage = "usage: tRead numBlocks";
  int numBlocks = 0;
  
  if (argc == 2)
    numBlocks = strtol (argv [1], NULL, 10);
  if (argc != 2 || (numBlocks == 0 && errno == EINVAL)) {
    printf ("%s\n", usage);
    return EXIT_FAILURE;
  }
  
  uthread_init (1);
  queue_init(&prq);  // initialize ready queue to put in threads
  disk_start   (interruptServiceRoutine);
  
  run (numBlocks);
  
  printf ("%d\n", sum);
}