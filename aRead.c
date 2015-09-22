#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/errno.h>
#include <assert.h>
#include "uthread.h"
#include "queue.h"
#include "disk.h"

int sum = 0;

/**
 * Description of 1 pending read.
 * Each pending read has one of these stored on the prq queue.
 */
struct PendingRead {
    char* buffer;
    int nbytes;
    int blockno;
    void (*handler) (char*, int, int);
    
  // TODO
    //the fields needed for asyncRead
};

/**
 * Queue of pending reads.
 */
queue_t prq;

void interruptServiceRoutine () {
  // called when disk fires an interrupt TODO
    
    struct PendingRead* pendingread = queue_dequeue(&prq);  //actually dequeues pendingread
    pendingread->handler (pendingread->buffer, pendingread->nbytes, pendingread->blockno); //change the value of buf
    
}

void asyncRead (char* buf, int nbytes, int blockno, void (*handler) (char*, int, int)) {
  // call disk_scheduleRead to schedule a read TODO
    struct PendingRead* pendingread = malloc (sizeof (struct PendingRead));
    // free this somewhere?
    pendingread->buffer = buf;
    pendingread->nbytes = nbytes;
    pendingread->blockno = blockno;
    pendingread->handler = handler;
    
    queue_enqueue(&prq, pendingread); // add pendingingread onto queue
    disk_scheduleRead(buf, nbytes, blockno); //schedule the read from disk
    
    
    
    
    }

/**
 * This is the read completion routine.  This is where you 
 * would place the code that processed the data you got back
 * from the disk.  In this case, we'll just verify that the
 * data is there.
 */
void handleRead (char* buf, int nbytes, int blockno) {
  assert (*((int*)buf) == blockno);
  sum += *(((int*) buf) + 1);
}

/**
 * Read numBlocks blocks from disk sequentially starting at block 0.
 */
void run (int numBlocks) {
  for (int blockno = 0; blockno < numBlocks; blockno++) {
      char buf[1000];
      asyncRead(buf, sizeof(buf), blockno, handleRead);
    // call asyncRead to schedule read TODO
  }
  disk_waitForReads(); //the call to read is faster than the actual read of the disk so you need to wait or the procedure will exit before the disk reads are done
}

int main (int argc, char** argv) {
  static const char* usage = "usage: aRead numBlocks";
  int numBlocks = 0;
  
  if (argc == 2)
    numBlocks = strtol (argv [1], NULL, 10);
  if (argc != 2 || (numBlocks == 0 && errno == EINVAL)) {
    printf ("%s\n", usage);
    return EXIT_FAILURE;
  }
  
  uthread_init (1);   // initialized thread
    queue_init (&prq);  //initialized queue
  disk_start   (interruptServiceRoutine);
  
  run (numBlocks);
  
  printf ("%d\n", sum);
}