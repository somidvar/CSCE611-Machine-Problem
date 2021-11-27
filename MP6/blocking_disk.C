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
    while (!permitted()) {
        SimpleDisk::issue_operation(DISK_OPERATION::READ, _block_no);
        while (!SimpleDisk::is_ready()) {
            Scheduler::currentScheduler->add(Thread::CurrentThread());
            Scheduler::currentScheduler->yield();
        }
        SimpleDisk::bufGetter(_buf);
    }
}

void BlockingDisk::write(unsigned long _block_no, unsigned char* _buf) {
    SimpleDisk::issue_operation(DISK_OPERATION::WRITE, _block_no);
    while (permitted()) {
        while (!SimpleDisk::is_ready()) {
            Scheduler::currentScheduler->add(Thread::CurrentThread());
            Scheduler::currentScheduler->yield();
        }
        SimpleDisk::bufSetter(_buf);
    }
}
bool BlockingDisk::isReady() {
    return is_ready();
}
bool BlockingDisk::permitted() {
    if(locking(SimpleDisk::lockFlag)==0){
        SimpleDisk::lockFlag=1;
        return true;
    }
    Scheduler::currentScheduler->add(Thread::CurrentThread());
    Scheduler::currentScheduler->yield();
    return false;
}