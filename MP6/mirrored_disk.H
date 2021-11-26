#ifndef MIRROREDDISK_H
#define MIRROREDDISK_H

#define MB *(0x1 << 20)
#define KB *(0x1 << 10)

#define SYSTEM_DISK_SIZE (10 MB)
#define DISK_BLOCK_SIZE ((1 KB) / 2)

#include "assert.H"
#include "blocking_disk.H"
#include "console.H"
#include "scheduler.H"
#include "simple_disk.H"
#include "thread.H"
#include "utils.H"

class MirroredDisk: public SimpleDisk {
   public:
    MirroredDisk(DISK_ID _disk_id,unsigned int _size);
    //Constructor which makes disk1 and disk2 to point to master and dependant.

    // virtual void yield();
    /* Called by the currently running thread in order to give up the CPU. 
      The scheduler selects the next thread from the ready queue to load onto 
      the CPU, and calls the dispatcher function defined in 'Thread.H' to
      do the context switch. */

    void read(unsigned long _block_no,unsigned char* _buf);
    void write(unsigned long _block_no,unsigned char* _buf);
};

#endif
