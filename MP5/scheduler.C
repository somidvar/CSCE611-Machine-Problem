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
#include "scheduler.H"
#include "simple_keyboard.H"
#include "thread.H"
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
Scheduler *Scheduler::currentScheduler = NULL;

Scheduler::Scheduler() {
    readyQueue = new Queue();
    Thread* currentThread=Thread::CurrentThread();
    Scheduler::currentScheduler=this;
    Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield() {
    if(readyQueue->queueSize==0){
        Console::puts("MAYDAY at Scheduler yield function\n");
        assert(false);
    }
    currentThread=readyQueue->dequeue();
    Thread::dispatch_to(currentThread);
}

void Scheduler::resume(Thread* _thread) {
    add(_thread);
}

void Scheduler::add(Thread* _thread) {
    readyQueue->enqueue(_thread);
    readyQueue->tempNode=readyQueue->head;
    for(int i=0;i<readyQueue->queueSize;i++){
        Console::puti(readyQueue->tempNode->thr->ThreadId());
        readyQueue->tempNode=readyQueue->tempNode->next;
    }
}

void Scheduler::terminate(Thread* _thread) {
    threadFinder(_thread);
    readyQueue->pop(readyQueue->foundedNode);
}

void Scheduler::threadFinder(Thread* _thread){
    bool foundFlag=false;
    readyQueue->foundedNode=readyQueue->head;
    for(int i=0;i<readyQueue->queueSize;i++){
        if(readyQueue->foundedNode->thr->ThreadId()==_thread->ThreadId()){
            foundFlag=true;
            break;
        }
        readyQueue->foundedNode=readyQueue->foundedNode->next;
    }
    if(!foundFlag){
        Console::puts("MAYDAY at Scheduler threadFinder, no match\n");
        readyQueue->foundedNode=NULL;
        assert(false);
    }    
}
