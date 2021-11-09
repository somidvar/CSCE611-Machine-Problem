#ifndef QUEUE_H
#define QUEUE_H

#include "thread.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/
class Queue {


  /* The scheduler may need private members... */
  
public:
   Queue();
   /* Setup the scheduler. This sets up the ready queue, for example.
      If the scheduler implements some sort of round-robin scheme, then the 
      end_of_quantum handler is installed in the constructor as well. */

   /* NOTE: We are making all functions virtual. This may come in handy when
            you want to derive RRScheduler from this class. */
  
   void enqueue(Thread * _thread);
   /* Called by the currently running thread in order to give up the CPU. 
      The scheduler selects the next thread from the ready queue to load onto 
      the CPU, and calls the dispatcher function defined in 'Thread.H' to
      do the context switch. */

   Thread* dequeue();
   /* Add the given thread to the ready queue of the scheduler. This is called
      for threads that were waiting for an event to happen, or that have 
      to give up the CPU in response to a preemption. */

   bool isEmpty();
   /* Make the given thread runnable by the scheduler. This function is called
      after thread creation. Depending on implementation, this function may 
      just add the thread to the ready queue, using 'resume'. */

   bool isFull();
   Thread* front();
   Thread* rear();
   /* Remove the given thread from the scheduler in preparation for destruction
      of the thread. 
      Graciously handle the case where the thread wants to terminate itself.*/
  
};
#endif