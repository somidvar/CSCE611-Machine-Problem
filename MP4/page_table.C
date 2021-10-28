#include "assert.H"
#include "console.H"
#include "exceptions.H"
#include "page_table.H"
#include "paging_low.H"

PageTable *PageTable::current_page_table = NULL;
unsigned int PageTable::paging_enabled = 0;
ContFramePool *PageTable::kernel_mem_pool = NULL;
ContFramePool *PageTable::process_mem_pool = NULL;
unsigned long PageTable::shared_size = 0;
// VMPool* PageTable::head=NULL;

void PageTable::init_paging(ContFramePool *_kernel_mem_pool,
                            ContFramePool *_process_mem_pool,
                            const unsigned long _shared_size) {
    Console::puts("init_paging is started\n");

    kernel_mem_pool = _kernel_mem_pool;
    process_mem_pool = _process_mem_pool;
    shared_size = _shared_size;

    Console::puts("init_paging is ended\n");
}

PageTable::PageTable() {
    Console::puts("Entering the constructor\n");
    // for meta data the important first 12 bits are present, read/write,
    // supervisor/user, reserved, reserved, accessed, dirty, reserved, reserved,
    // available and available from begining to end
    if (!page_directory) {                                             // the page directory does not exist
        unsigned long directoryAdd = process_mem_pool->get_frames(1);  // getting a frame from the process to store the directory
        page_directory = (unsigned long *)(directoryAdd * PAGE_SIZE);
        unsigned long metaData;
        metaData = 0 + 2 + 0;  // not present,read/write and kernel mode
        for (int i = 0; i < 1024; i++) {
            page_directory[i] = metaData;  // initializing the directory entries
        }
        // mapping the first 4 MB
        unsigned long directMappingAdd = process_mem_pool->get_frames(1);  // getting a frame from the process to store the direct mapping
        directMapping = (unsigned long *)(directMappingAdd * PAGE_SIZE);
        metaData = 1 + 2 + 0;  // present, read/write and kernel
        for (unsigned long i = 0; i < 1024; i++) {
            directMapping[i] = (i * PAGE_SIZE) + metaData;
        }
        metaData = 1 + 2 + 0;                                           // present, read/write and kernel
        page_directory[0] = (directMappingAdd * PAGE_SIZE) + metaData;  // adding the address of the 4MB mapping as the first entries of the directory

        metaData = 1 + 2 + 0;                                          // present, read/write and kernel
        page_directory[1023] = (directoryAdd * PAGE_SIZE) + metaData;  //the last entry of the directory points to iteself

        Console::puts("Directory is created\n");
    }
    Console::puts("constructor is ended\n");
}

void PageTable::load() {
    // Console::puts("Entering load\n");
    current_page_table = this;
    write_cr3((unsigned long)page_directory);
    // Console::puts("Loaded page table\n");
}

void PageTable::enable_paging() {
    Console::puts("Entering enable_paging\n");
    write_cr0(read_cr0() | 0x80000000);
    Console::puts("Enabled paging\n");

    current_page_table->vmPoolBase = (unsigned long *)(1024 * 1024 * 4);
    current_page_table->vmPoolSize = (unsigned long *)(1024 * 1024 * 4 + 1024 * 4);
}

void PageTable::handle_fault(REGS *_r) {
    unsigned long pdeInfo, pteInfo, metaData;
    Console::puts("Entering handle_fault for add=");
    unsigned long faultCode = _r->err_code;
    if (faultCode & 1 != 1) {  // making sure that we are dealing with a page fault
        Console::puts("\nMAYDAY at handle_fault, checkpoint 1. fault_code=");
        Console::putui(faultCode);
        Console::puts("\n");
        assert(false);
    }
    unsigned long faultAdd = (unsigned long)read_cr2();  // reading the address issued by CPU
    Console::putui(faultAdd);
    Console::puts("\n");

    pdeInfo = PdeGetterSetter(faultAdd, 0, 0);
    if ((pdeInfo & 1) != 1) {  // the pde entry is not present
        Console::puts("PDE is not found\n");
        unsigned long newPageAdd = process_mem_pool->get_frames(1);
        metaData = 1 + 2 + 0;  // present, read/write and kernel
        newPageAdd = newPageAdd * PAGE_SIZE;
        PdeGetterSetter(faultAdd, metaData, newPageAdd);
        Console::puts("Page directory is created\n");
    }

    pteInfo = PteGetterSetter(faultAdd, 0, 0);
    if ((pteInfo & 1) != 1) {
        Console::puts("PTE is not found\n");
        unsigned long newPageAdd = process_mem_pool->get_frames(1);
        metaData = 1 + 2 + 4;  // present, read/write and user
        newPageAdd = newPageAdd * PAGE_SIZE;
        PteGetterSetter(faultAdd, metaData, newPageAdd);
        Console::puts("Page table is created\n");
    } else {
        Console::puts("MAYDAY at handle_fault, checkpoint 2, pte is present\n");
        assert(false);
    }
    Console::puts("handled page fault for add=");
    Console::putui(faultAdd);
    Console::puts(" and metadata=");
    Console::putui(PteGetterSetter(faultAdd,0,0));
    Console::puts("\n");
}

void PageTable::register_pool(VMPool *_vm_pool) {
    VMPool *vmPool = _vm_pool;
    Console::puts("PageTable::register_pool=");
    Console::putui(vmCounter);
    Console::puts("\n");
    vmPoolBase[vmCounter] = vmPool->vmBaseAddress;
    vmPoolSize[vmCounter] = vmPool->vmSize;
    vmCounter++;
    if (head == NULL) {
        head = vmPool;
        return;
    }
    VMPool *currentVMPool = head;
    while (currentVMPool->next != NULL)
        currentVMPool = currentVMPool->next;
    currentVMPool->next = vmPool;
}

void PageTable::free_page(unsigned long _page_no) {
    Console::puts("PageTable::free_page for page=");
    Console::putui(_page_no);
    Console::puts("\n");

    unsigned long pageAdd,pde,pte,metaData;
    pageAdd=_page_no*PAGE_SIZE;
    metaData=PteGetterSetter(pageAdd,0,0);
    Console::putui(metaData);
    if((metaData&1)==0){
        Console::puts("The page is already free*******************\n");
        return;
    }
    metaData=metaData>>12;
    Console::putui(metaData);
    process_mem_pool->release_frames(metaData);//releasing the frame from process pool

    metaData=0+2+0;//absent, read/write, kernel
    PteGetterSetter(pageAdd,metaData,0);//setting the page table entry to absent

    load();//refreshing the TLB
}

unsigned long PageTable::PdeGetterSetter(unsigned long byteAddress, unsigned long metaData, unsigned long pdeEntry) {
    unsigned long pde, pte, result, recursivePDAdd, pageTableAdd;
    pde = byteAddress >> 22;
    pte = byteAddress << 10;
    pte = pte >> 22;
    recursivePDAdd = (1023 << 22) + (1023 << 12);
    unsigned long *recursivePDData = (unsigned long *)(recursivePDAdd);
    if (metaData == 0)
        return recursivePDData[pde];
    recursivePDData[pde] = metaData + pdeEntry;
    pageTableAdd = (1023 << 22) + (pde << 12);
    PageInitilizer(pageTableAdd);
    return recursivePDData[pde];
}
unsigned long PageTable::PteGetterSetter(unsigned long byteAddress, unsigned long metaData, unsigned long pteEntry) {
    unsigned long pde, pte, result, recursivePTAdd, pageTablePageAdd;
    pde = byteAddress >> 22;
    pte = byteAddress << 10;
    pte = pte >> 22;
    recursivePTAdd = (1023 << 22) + (pde << 12);
    unsigned long *recursivePTData = (unsigned long *)(recursivePTAdd);
    if (metaData == 0)
        return recursivePTData[pte];
    recursivePTData[pte] = metaData + pteEntry;
    return recursivePTData[pte];
}
void PageTable::PageInitilizer(unsigned long pageAdd) {
    unsigned long *pageData = (unsigned long *)(pageAdd);
    unsigned long metaData = 0 + 2 + 0;  //absent, read/write, kernel
    for (int i = 0; i < 1024; i++)
        pageData[i] = metaData;
}