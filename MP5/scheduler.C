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




Scheduler::Scheduler() {
    // Queue jafar;
    // int a = 100;
    // int b = 200;
    // int c = 300;
    // int d = 400;

    // jafar.enqueue(&a);
    // {
    //     Node* testN = jafar.head;
    //     int* tempVal = testN->temp;
    //     Console::puti(tempVal[0]);
    //     Console::puts("\n");
    // }
    // jafar.enqueue(&b);
    // {
    //     Node* testN = jafar.head;
    //     int* tempVal = testN->temp;
    //     Console::puti(tempVal[0]);
    //     Console::puts("\n");
    // }
    // jafar.enqueue(&c);
    // jafar.enqueue(&d);

    // Node* testN = jafar.head;
    // for (int i = 0; i < 4; i++) {
    //     int* tempVal = testN->temp;
    //     Console::puti(tempVal[0]);
    //     Console::puts("\n");
    //     testN = testN->next;
    // }

    readyQueue = new Queue();
    Thread* currentThread=Thread::CurrentThread();
    Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield() {

    
    futureThread=readyQueue->dequeue();
    // currentThread->dispatch_to(futureThread);
    // currentThread=futureThread;
    


    
}

void Scheduler::resume(Thread* _thread) {
    add(_thread);
        
}

void Scheduler::add(Thread* _thread) {
    readyQueue->enqueue(_thread);
}

void Scheduler::terminate(Thread* _thread) {
    assert(false);
}
