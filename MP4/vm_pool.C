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
    vmBaseAddress = _base_address;
    vmSize = _size;
    pageTable = _page_table;
    contFramePool = _frame_pool;

    unsigned long logicalRange = 1024 * 1024 * 1024 * 4 - 1;
    if (vmSize + vmBaseAddress >= logicalRange) {  //bigger than the max of the logical space
        Console::puts("MAYDAY at VMPool constructor, checkpoint 1\n");
        assert(false);
    }

    for (int i = 0; i < pageTable->vmCounter; i++) {
        if (vmBaseAddress >= pageTable->vmPoolBase[i] && vmBaseAddress <= pageTable->vmPoolSize[i]) {  //thre is an interference between this pool and another one
            Console::puts("MAYDAY at VMPool constructor, checkpoint 2\n");
            assert(false);
        }
    }
    allocatedStart = (unsigned long *)(vmBaseAddress);           //pointing it to the base of the pool
    allocatedEnd = (unsigned long *)(vmBaseAddress + 4 * 1024);  //pointing it to the base of the pool+4 KB
    tempPtr = (unsigned long *)(vmBaseAddress + 8 * 1024);       //pointing to the base of the pool+8 KB
    regionCounter = 0;

    //no region is in the allocated list
    for (int i = 0; i < 1024; i++) {
        allocatedStart[i] = -1;
        allocatedEnd[i] = -1;
    }
    Console::puts("****************************************\n");
    Console::putui(vmBaseAddress);
    Console::puts("****************************************\n");
    //adding the allocatedStart to the allocated list
    allocatedStart[0] = vmBaseAddress;
    allocatedEnd[0] = vmBaseAddress + 4 * 1024;
    regionCounter++;

    //adding the allocatedEnd to the allocated list
    allocatedStart[1] = vmBaseAddress + 4 * 1024;
    allocatedEnd[1] = vmBaseAddress + 8 * 1024;
    regionCounter++;

    //adding the temp to the allocated list
    allocatedStart[2] = vmBaseAddress + 8 * 1024;
    allocatedEnd[2] = vmBaseAddress + 12 * 1024;
    regionCounter++;

    pageTable->register_pool(this);
    Console::puts("VMPool constructor ends\n");
}

unsigned long VMPool::allocate(unsigned long _size) {
    Console::puts("VMPool::allocate starts\n");
    unsigned long regionSize = _size;    //the region size is in bytes
    if (regionSize % (4 * 1024) != 0) {  //rounding to a page which causes internal fragmentation. This makes the code much easier
        regionSize = regionSize >> 12;
        regionSize += 1;
        regionSize << 12;
    }
    Console::putui(regionSize);
    unsigned long regionBaseAdd = emptySpaceFinder(regionSize);
    if (regionBaseAdd < 0) {
        Console::puts("MAYDAY at VMPool::allocate checkpoint 3\n");
        assert(false);
    }
    allocatedStart[regionCounter] = regionBaseAdd;
    allocatedEnd[regionCounter] = regionBaseAdd + regionSize;
    regionCounter++;
    allocatedListSorter();
    Console::puts("VMPool::allocate ends\n");
}

void VMPool::release(unsigned long _start_address) {
    Console::puts("VMPool::release function starts.\n");
    unsigned long startAddress = _start_address;
    if (!is_legitimate(startAddress)) {
        Console::puts("MAYDAY at VMPool::release checkpoint 1\n");
        assert(false);
    }
    bool foundFlag = false;
    unsigned long foundIndex;
    for (int i = 0; i < regionCounter; i++) {  //finding the region to be released
        if (allocatedStart[i] == _start_address) {
            foundFlag = true;
            foundIndex = i;
            break;
        }
    }
    if (!foundFlag) {
        Console::puts("MAYDAY at VMPool::release checkpoint 2\n");
        assert(false);
    }
    
    swapFunc(&allocatedStart[foundIndex], &allocatedStart[regionCounter - 1]);
    swapFunc(&allocatedEnd[foundIndex], &allocatedEnd[regionCounter - 1]);
    regionReleaseAux(allocatedStart[foundIndex], allocatedEnd[foundIndex]);

    allocatedStart[regionCounter - 1] = -1;
    allocatedEnd[regionCounter - 1] = -1;
    regionCounter--;
    allocatedListSorter();
    Console::puts("VMPool::release function ends.\n");
}

void VMPool::regionReleaseAux(unsigned long start, unsigned long end) {
    unsigned long pageStart, pageEnd;
    pageStart = start << 12;
    pageEnd = end << 12;
    for (int i = pageStart; i < pageEnd; i++)
        pageTable->free_page(i);
}

bool VMPool::is_legitimate(unsigned long _address) {
    return true;
    ///////////////////////////////////////////---------------------------fix
    // Console::puts("VMPool::is_legitimate\n");
    // if (_address >= vmBaseAddress && _address <= (vmBaseAddress + vmSize))
    //     return true;
    // return false;
}

unsigned long VMPool::emptySpaceFinder(unsigned long _size) {
    unsigned long requestedSize = _size;
    unsigned long baseAddress;
    bool foundFlag = false;

    for (int i = 0; i < regionCounter-1; i++) {
        if (allocatedStart[i + 1] - allocatedEnd[i] >= requestedSize) {
            baseAddress = allocatedEnd[i];
            return baseAddress;
        }
    }
    //could not find any hole between the regions, so we have to check the space between the last region to the end of the pool
    if(vmSize+vmBaseAddress -allocatedEnd[regionCounter-1]>requestedSize){
        baseAddress=allocatedEnd[regionCounter-1];
        return baseAddress;
    }
    if (!foundFlag) {
        Console::puts("MAYDAY at VMPool::emptySpaceFinder checkpoint 2\n");
        assert(false);
    }
    return -1;
}
void VMPool::allocatedListSorter() {  //sorting allocatedStart and allocatedEnd using selection sort
    unsigned tempVal;
    for (int i = 0; i < regionCounter; i++) {
        for (int j = i; j < regionCounter; j++) {
            if (allocatedStart[i] > allocatedStart[j]) {  //doing a swap only based on the allocatedStart
                swapFunc(&allocatedStart[i], &allocatedStart[j]);
                swapFunc(&allocatedEnd[i], &allocatedEnd[j]);
            }
        }
    }
}

void VMPool::swapFunc(unsigned long *a, unsigned long *b) {  //swapping two elemnt
    unsigned long tempVal = *a;
    *a = *b;
    *b = tempVal;
}

void VMPool::testOnly() {
    // Console::puts("\n==========================================\n");

    // allocatedStart[3]=5*4*1024*1024;
    // allocatedStart[4]=8*4*1024*1024;
    // allocatedStart[5]=10*4*1024*1024;
    // allocatedStart[6]=1*4*1024;
    // allocatedStart[7]=6*4*1024;
    // allocatedStart[8]=9*4*1024;
    // allocatedStart[9]=18*4*1024;

    // regionCounter=10;
    // Console::puts("\n==========================================\n");
    // for(int i=0;i<regionCounter;i++)
    //     Console::putui(allocatedStart[i]);
    // Console::puts("\n==========================================\n");
    // allocatedListSorter();
    // for(int i=0;i<regionCounter;i++)
    //     Console::putui(allocatedStart[i]);












    // allocate(50 * 1024 * 1024);
    // Console::puts("==========================================\n");
    // for (int i = 0; i < regionCounter; i++) {
    //     Console::putui(allocatedStart[i]/1024/1024);
    // }
    // Console::puts("\n");
    // for (int i = 0; i < regionCounter; i++) {
    //     Console::putui(allocatedEnd[i]/1024/1024);
    // }
    // allocate(100 * 1024 * 1024);

    //     Console::puts("==========================================\n");
    // for (int i = 0; i < regionCounter; i++) {
    //     Console::putui(allocatedStart[i]/1024/1024);
    // }
    // Console::puts("\n");
    // for (int i = 0; i < regionCounter; i++) {
    //     Console::putui(allocatedEnd[i]/1024/1024);
    // }


    // allocate(10 * 1024 * 1024);
    
    // Console::puts("\n==========================================\n");
    // for (int i = 0; i < regionCounter; i++) {
    //     Console::putui(allocatedStart[i]/1024/1024);
    // }
    // Console::puts("\n");
    // for (int i = 0; i < regionCounter; i++) {
    //     Console::putui(allocatedEnd[i]/1024/1024);
    // }

    // release(2148364*1024);

    // Console::puts("\n==========================================\n");
    // for (int i = 0; i < regionCounter; i++) {
    //     Console::putui(allocatedStart[i]/1024/1024);
    // }
    // Console::puts("\n");
    // for (int i = 0; i < regionCounter; i++) {
    //     Console::putui(allocatedEnd[i]/1024/1024);
    // }


    // allocate(1 * 1024 * 1024);
    // allocate(98 * 1024 * 1024);

    // Console::puts("\n==========================================\n");
    // for (int i = 0; i < regionCounter; i++) {
    //     Console::putui(allocatedStart[i]/1024/1024);
    // }
    // Console::puts("\n");
    // for (int i = 0; i < regionCounter; i++) {
    //     Console::putui(allocatedEnd[i]/1024/1024);
    // }










    


    // assert(false);

    // Console::puts("==========================================\n");
    // Console::putui(regionCounter);
    // for(int i=0;i<regionCounter;i++){
    //     Console::putui(allocatedStart[i]/1024);
    //     Console::putui(allocatedEnd[i]/1024);
    // }
    // allocate(10*1024*1024);
    // Console::puts("\n***********************\n");
    // Console::putui(regionCounter);
    // for(int i=0;i<regionCounter;i++){
    //     Console::putui(allocatedStart[i]/1024);
    //     Console::putui(allocatedEnd[i]/1024);
    // }
}