#include "queue.H"
#include "thread.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"



Queue::Queue(){
    assert(false)
    Console::puts("Constructed Scheduler.\n");
}

void Queue::enqueue(Thread* _thread) {
    assert(false);
}

Thread* Queue::dequeue() {
    assert(false);
}

bool Queue::isEmpty() {
    assert(false);
}

bool Queue::isFull() {
    assert(false);
}
Thread* Queue::front() {
    assert(false);
}
Thread* Queue::rear() {
    assert(false);
}
