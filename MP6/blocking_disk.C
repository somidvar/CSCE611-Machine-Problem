/*
     File        : blocking_disk.c

     Author      : 
     Modified    : 

     Description : 

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "blocking_disk.H"
#include "lock_disk.H"

// extern int locking(int lockFlag);
// extern void lockRelease(bool lockFlag);

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

BlockingDisk::BlockingDisk(DISK_ID _disk_id, unsigned int _size)
    : SimpleDisk(_disk_id, _size) {
}

/*--------------------------------------------------------------------------*/
/* SIMPLE_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/

void BlockingDisk::read(unsigned long _block_no, unsigned char* _buf) {
    bool opDone = false;
    while (!opDone) {
        if (permitted())
            opDone = true;
        else {
            sleep();
        }
    }
    SimpleDisk::issue_operation(DISK_OPERATION::READ, _block_no);
    while (!SimpleDisk::is_ready()) {
        sleep();
    }
    SimpleDisk::bufGetter(_buf);
    SimpleDisk::unlock();
    Console::puts("Reading is done\n");
}

void BlockingDisk::write(unsigned long _block_no, unsigned char* _buf) {
    bool opDone = false;
    while (!opDone) {
        if (permitted())
            opDone = true;
        else {
            sleep();
        }
    }
    SimpleDisk::issue_operation(DISK_OPERATION::WRITE, _block_no);
    while (!SimpleDisk::is_ready()) {
        sleep();
    }
    SimpleDisk::bufSetter(_buf);
    SimpleDisk::unlock();
    Console::puts("Writting is done\n");
}
bool BlockingDisk::permitted() {
    if (locking(SimpleDisk::lockFlag) == 0) {
        SimpleDisk::lockFlag = 1;
        return true;
    }
    return false;
}
void BlockingDisk::sleep() {
    Scheduler::currentScheduler->add(Thread::CurrentThread());
    Scheduler::currentScheduler->yield();
}