/*
 File: vm_pool.C
 
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
#include "simple_keyboard.H"
#include "utils.H"
#include "vm_pool.H"

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
/* METHODS FOR CLASS   V M P o o l */
/*--------------------------------------------------------------------------*/

VMPool::VMPool(unsigned long _base_address, unsigned long _size, ContFramePool *_frame_pool, PageTable *_page_table) {
    Console::puts("VMPool constructor starts\n");
    vmBaseAddress=_base_address;
    vmSize=_size;
    pageTable=_page_table;
    contFramePool=_frame_pool;
    

    unsigned long range=1024*1024*1024*4-1;

    for(int i=0;i<pageTable->vmCounter;i++){
        if(vmBaseAddress>=pageTable->vmPoolBase[i] && vmBaseAddress<=pageTable->vmPoolSize[i]){//thre is an interference between this pool and another one
            Console::puts("MAYDAY code 2 in VMPool::allocate.\n");
            assert(false);
        }
    }
    pageTable->register_pool(this);
    Console::puts("VMPool constructor ends\n");
}

unsigned long VMPool::allocate(unsigned long _size) {
    Console::puts("VMPool::allocate starts\n");
    
    Console::puts("VMPool::allocate ends\n");
}

void VMPool::release(unsigned long _start_address) {
    assert(false);
    Console::puts("Released region of memory.\n");
}

bool VMPool::is_legitimate(unsigned long _address) {
    Console::puts("VMPool::is_legitimate\n");
    if(_address>=vmBaseAddress && _address<=(vmBaseAddress+vmSize))
        return true;
    return false;
    
}
