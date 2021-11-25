/*
 File: scheduler.C
 
 Author:
 Date  :
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "mem_pool.H"
#include "scheduler.H"
#include "simple_keyboard.H"
#include "utils.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

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
Scheduler* Scheduler::currentScheduler = NULL;

Scheduler::Scheduler() {
    readyQueue = new Queue();
    Scheduler::currentScheduler = this;
    Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield() {
    if (readyQueue->queueSize == 0) {//there is no thread to pass the control to.
        Console::puts("MAYDAY at Scheduler yield function\n");
        assert(false);//this is optional for the sake of debuging
        return;
    }
    Thread* newThread = readyQueue->dequeue();
    Thread::dispatch_to(newThread);
}

void Scheduler::resume(Thread* _thread) {
    add(_thread);
}

void Scheduler::add(Thread* _thread) {
    readyQueue->enqueue(_thread);
}

void Scheduler::terminate(Thread* _thread) {
    if(readyQueue->queueSize==0){//we don't have any other threads to pass the control to!
        Console::puts("MAYDAY at terminate\n");
        assert(false);//this is optional for the sake of debuging
        return;
    }
    if (threadFinder(_thread)){//finding and removing the terminated thread from the readyqueue
        readyQueue->pop(readyQueue->foundedNode);
    }
    if(_thread!=NULL){//removing the stack of the terminated thread to avoid memory leak
        stackRemover(_thread->getStack());
        if(_thread->ThreadId()== Thread::CurrentThread()->ThreadId())//if it is suicide, we have to pass the control
            yield();//anything after this line will not get executed
    }
}

bool Scheduler::threadFinder(Thread* _thread) {//if the thread is found true;otherwise, false. The found thread will be put in the foundThread
    readyQueue->foundedNode = readyQueue->head;
    for (int i = 0; i < readyQueue->queueSize; i++) {
        if (readyQueue->foundedNode->thr->ThreadId() == _thread->ThreadId()) {
            return true;
        }
        readyQueue->foundedNode = readyQueue->foundedNode->next;
    }
    readyQueue->foundedNode = NULL;
    return false;
}

void Scheduler::stackRemover(char* threadStack) {//releasing the thread stack
    Console::puts("Releasing thread stack\n");
    delete[] threadStack;
}