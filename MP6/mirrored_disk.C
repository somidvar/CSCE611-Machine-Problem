#include "mirrored_disk.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

MirroredDisk::MirroredDisk(DISK_ID _disk_id,unsigned int _size):SimpleDisk(_disk_id,_size){
    Console::puts("MirroredDisk cons\n");
}
void MirroredDisk::read(unsigned long _block_no, unsigned char* _buf) {
    SimpleDisk::disk_id=DISK_ID::MASTER;
    SimpleDisk::issue_operation(DISK_OPERATION::READ,_block_no);

    SimpleDisk::disk_id=DISK_ID::DEPENDENT;
    SimpleDisk::issue_operation(DISK_OPERATION::READ,_block_no);

    while(!(SimpleDisk::is_ready())){
        Scheduler::currentScheduler->add(Thread::CurrentThread());
        Scheduler::currentScheduler->yield();
    }
    SimpleDisk::bufGetter(_buf);
    // assert(false); 
}
void MirroredDisk::write(unsigned long _block_no, unsigned char* _buf) {
    SimpleDisk::disk_id=DISK_ID::MASTER;
    SimpleDisk::issue_operation(DISK_OPERATION::WRITE,_block_no);
    while(!(SimpleDisk::is_ready())){
        Scheduler::currentScheduler->add(Thread::CurrentThread());
        Scheduler::currentScheduler->yield();
    }
    SimpleDisk::bufSetter(_buf);

    SimpleDisk::disk_id=DISK_ID::DEPENDENT;
    SimpleDisk::issue_operation(DISK_OPERATION::WRITE,_block_no);
        while(!(SimpleDisk::is_ready())){
        Scheduler::currentScheduler->add(Thread::CurrentThread());
        Scheduler::currentScheduler->yield();
    }
    SimpleDisk::bufSetter(_buf);
}