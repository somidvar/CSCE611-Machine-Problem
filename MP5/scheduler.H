/* 
    Author: R. Bettati, Joshua Capehart
            Department of Computer Science
            Texas A&M University
			
	    A thread scheduler.

*/
#ifndef SCHEDULER_H
#define SCHEDULER_H

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "thread.H"

/*--------------------------------------------------------------------------*/
/* !!! IMPLEMENTATION HINT !!! */
/*--------------------------------------------------------------------------*/
/*
    One way to proceed is to implement the FIFO scheduling policy inside
    class 'Scheduler'. 

    If you plan to implement a Round-Robin Scheduler, derive it from class
    'Scheduler', say as class 'RRScheduler'. The class 'RRScheduler' is 
    really just a FIFO scheduler with THREE MODIFICATIONS:
    1. It manages a timer, which fires at the end-of-quantum (EOQ). 
    (For details on how to set up a timer and how to handle timer interrupts 
    see the 1-second timer in 'kernel.C'.)  The timer is set up in the
    constructor.
    2. It uses an additional function, the EOQ handler. This function gets
    called whenever an EOQ timer event fires. The EOQ handler forces the 
    current thread to call the scheduler's 'yield' function.
    3. The 'yield' function must be modified to account for unused quantum
    time. If a thread voluntarily yields, the EOQ timer must be reset in order
    to not penalize the next thread.
 
    (Note that this qualifies as programming at about the level of a baboon.
     Much better woudl be to have the abstract class 'Scheduler' implement 
     the basic scheduling MECHANISMS and provide abstract funtions to define
     the queue management POLICIES in derived classes, 
     such as 'FIFOScheduler'.)
    
 */

/*--------------------------------------------------------------------------*/
/* SCHEDULER */
/*--------------------------------------------------------------------------*/

class Scheduler {
    /* The scheduler may need private members... */

   public:
    struct Node {
        Thread* thr;
        int timeQuantum = 0;
        Node* next;
        Node* prev;
    };

    struct Queue {
        int queueSize = 0;
        Node* tail = NULL;
        Node* head = NULL;
        Node* foundedNode = NULL;
        Node* tempNode = NULL;

        void enqueue(Thread* _thread) {
            struct Node* newElement = new Node();
            newElement->thr = _thread;
            if (head == NULL && tail == NULL) {
                head = newElement;
                tail = newElement;
                head->prev = NULL;
                head->next = NULL;
            } else {
                tail->next = newElement;
                newElement->prev = tail;
                tail = newElement;
            }
            queueSize += 1;
        }

        Thread* dequeue() {
            if (queueSize == 0) {
                Console::puts("MAYDAY AT Scheduler dequeue function, the queue is empty\n");
                assert(false)
            }
            Node* pred = head;
            Thread* threadDequeue = head->thr;
            if (queueSize == 1) {
                head = NULL;
                tail = NULL;
                delete[] pred;
                queueSize--;
                return threadDequeue;
            }
            head = head->next;
            head->prev = NULL;
            queueSize--;
            delete[] pred;
            return threadDequeue;
        }
        void pop(Node* _deleteNode) {
            Node* deleted = _deleteNode;
            Node* previousNode = NULL;
            Node* nextNode = NULL;
            if (deleted->prev == NULL && deleted->next == NULL) {  //only node
                Console::puts("case1, the queue size is now=");
                head = NULL;
                tail = NULL;
                delete[] deleted;
                queueSize--;
                Console::puti(queueSize);
                Console::puts("\n");
                return;
            }
            if (deleted->next == NULL) {  //tail node
                Console::puts("case2, the queue size is now=");
                previousNode = deleted->prev;
                previousNode->next = NULL;
                tail = previousNode;
                delete[] deleted;
                queueSize--;
                Console::puti(queueSize);
                Console::puts("\n");
                return;
            }
            if (deleted->prev == NULL) {  //head node
                Console::puts("case3, the queue size is now=");
                nextNode = deleted->next;
                nextNode->prev = NULL;
                head = nextNode;
                delete[] deleted;
                queueSize--;
                Console::puti(queueSize);
                Console::puts("\n");
                return;
            }
            Console::puts("case4, the queue size is now=");
            previousNode = deleted->prev;
            nextNode = deleted->next;

            previousNode->next = nextNode;
            nextNode->prev = previousNode;
            queueSize--;

            delete[] deleted;
            Console::puti(queueSize);
            Console::puts("\n");
        }
    };

    Scheduler();
    /* Setup the scheduler. This sets up the ready queue, for example.
      If the scheduler implements some sort of round-robin scheme, then the 
      end_of_quantum handler is installed in the constructor as well. */

    /* NOTE: We are making all functions virtual. This may come in handy when
            you want to derive RRScheduler from this class. */

    virtual void yield();
    /* Called by the currently running thread in order to give up the CPU. 
      The scheduler selects the next thread from the ready queue to load onto 
      the CPU, and calls the dispatcher function defined in 'Thread.H' to
      do the context switch. */

    virtual void resume(Thread* _thread);
    /* Add the given thread to the ready queue of the scheduler. This is called
      for threads that were waiting for an event to happen, or that have 
      to give up the CPU in response to a preemption. */

    virtual void add(Thread* _thread);
    /* Make the given thread runnable by the scheduler. This function is called
      after thread creation. Depending on implementation, this function may 
      just add the thread to the ready queue, using 'resume'. */

    virtual void terminate(Thread* _thread);
    /* Remove the given thread from the scheduler in preparation for destruction
      of the thread. 
      Graciously handle the case where the thread wants to terminate itself.*/
    virtual void threadFinder(Thread* _thread);
    /* Finding the thread based on the thread ID in the readyQueue and placing it
    in foundedNode.
    */

    static Scheduler* currentScheduler;

   private:
    Queue* readyQueue;      //this is the queue for scheduling
    Thread* currentThread;  //this is the current running thread
};

#endif
