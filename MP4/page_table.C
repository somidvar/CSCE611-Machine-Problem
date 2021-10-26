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
        metaData = 1 * 0 + 2 * 1 + 4 * 0;  // not present,read/write and kernel mode
        for (int i = 0; i < 1024; i++) {
            page_directory[i] = metaData;  // initializing the directory entries
        }
        // mapping the first 4 MB
        unsigned long directMappingAdd = process_mem_pool->get_frames(1);  // getting a frame from the process to store the direct mapping
        directMapping = (unsigned long *)(directMappingAdd * PAGE_SIZE);
        metaData = 1 * 1 + 2 * 1 + 4 * 0;  // present, read/write and kernel
        for (unsigned long i = 0; i < 1024; i++) {
            directMapping[i] = (i * PAGE_SIZE) + metaData;
        }
        metaData = 1 * 1 + 2 * 1 + 4 * 0;                               // present, read/write and kernel
        page_directory[0] = (directMappingAdd * PAGE_SIZE) + metaData;  // adding the address of the 4MB mapping as the first entries of the directory

        metaData = 1 * 1 + 2 * 1 + 4 * 0;                              // present, read/write and kernel
        page_directory[1023] = (directoryAdd * PAGE_SIZE) + metaData;  //the last entry of the directory points to iteself

        Console::puts("Directory is created\n");
    }

    Console::puts("constructor is ended\n");
}

void PageTable::load() {
    Console::puts("Entering load\n");
    current_page_table = this;
    write_cr3((unsigned long)page_directory);
    Console::puts("Loaded page table\n");
}

void PageTable::enable_paging() {
    Console::puts("Entering enable_paging\n");
    write_cr0(read_cr0() | 0x80000000);
    Console::puts("Enabled paging\n");

    current_page_table->vmPoolBase = (unsigned long *)(1024 * 1024 * 4);
    current_page_table->vmPoolSize = (unsigned long *)(1024 * 1024 * 4 + 1024 * 4);
}

void PageTable::handle_fault(REGS *_r) {
    unsigned long *recursivePD = (unsigned long *)0xFFFFF000;  // this is equivalent to the address of 1023|1023|0
    unsigned long metaData, pageMetaData, pde, pte;            // page metadata, page directory entry, page table entry
    unsigned long pageTableAdd;
    Console::puts("Entering handle_fault for add=");
    unsigned long faultCode = _r->err_code;
    if (faultCode & 1 != 1) {  // making sure that we are dealing with a page fault
        Console::puts("\nMAYDAY at handle_fault, the fault code is not equal to not present. Code=");
        Console::putui(faultCode);
        Console::puts("\n");
        assert(false);
    }
    unsigned long faultAdd = (unsigned long)read_cr2();  // reading the address issued by CPU
    Console::putui(faultAdd);
    Console::puts("\n");

    pde = faultAdd >> 22;  // removing metadata and pte
    pte = faultAdd << 10;  // removing the pde
    pte = pte >> 10;       //resotring to the original place
    pte = pte >> 12;       //removing the metadata

    // Console::puts("PDE and PTE=");
    // Console::putui(pde);
    // Console::putui(pte);
    // Console::puts("\n");
    pageMetaData = recursivePD[pde];
    pageMetaData = pageMetaData << 20;                  //removing pde and pte
    pageMetaData = pageMetaData >> 20;                  //restoring to the original place
    unsigned long pdeFlag = pageMetaData & 0x00000001;  // if the directory has a record of the page

    if (pdeFlag == 0) {  // the directory entry is not present
        // Console::puts("The directory entry is not present\n");
        unsigned long newPageAdd = process_mem_pool->get_frames(1);
        // Console::putui(newPageAdd);
        metaData = 1 + 2 + 0;  // present, read/write and kernel

        recursivePD[pde] = newPageAdd * PAGE_SIZE + metaData;
        newPageAdd = newPageAdd >> 10;
        newPageAdd += 1023 << 10;  //this is page tabe entry which is located at 1023|frameNumber|0
        unsigned long *newPageData = (unsigned long *)(newPageAdd * PAGE_SIZE);
        metaData = 0 + 2 + 0;  // absent, read/write and kernel
        for (int i = 0; i < 1024; i++) {
            newPageData[i] = metaData;
        }
        // Console::puts("Page directory is created\n");
    }

    pde = faultAdd >> 22;               // removing metadata and pte
    pte = faultAdd << 10;               // removing the pde
    pte = pte >> 10;                    //resotring to the original place
    pte = pte >> 12;                    //removing the metadata
    pageTableAdd = (1023 << 10) + pde;  // the pde is set to 1023 and pte is set to fault address pde
    unsigned long *pageTablePage = (unsigned long *)(pageTableAdd * PAGE_SIZE);
    pageMetaData = pageTablePage[pte];
    pageMetaData = pageMetaData << 20;         //removing pde and pte
    pageMetaData = pageMetaData >> 20;         //restoring to the original place
    pageMetaData = pageMetaData & 0x00000001;  // if the page is present at the page table page
    if (pageMetaData == 0) {                   // the directory entry is present but the page is not
        // Console::puts("The page table entry is not present\n");
        unsigned long newFrameAdd = process_mem_pool->get_frames(1);  // this time we get a frame from the prcoess to sit the actual data
        metaData = 1 + 2 + 4;                                         // present, read/write and user
        pageTablePage[pte] = newFrameAdd * PAGE_SIZE + metaData;
    } else {  //not sure why the page fault is happening
        Console::puts("MAYDAY at handle_fault with address=");
        Console::putui(faultAdd);
        Console::putui(pageMetaData);
        Console::puts("\n");
        assert(false);
    }
    // Console::puts("handled page fault for add=");
    // Console::putui(faultAdd);
    // Console::puts("\n");
 
}

void PageTable::register_pool(VMPool *_vm_pool) {
    vmPoolBase[vmCounter] = _vm_pool->vmBaseAddress;
    vmPoolSize[vmCounter] = _vm_pool->vmSize;
    vmCounter++;
    Console::puts("PageTable::register_pool=");
    Console::putui(vmCounter);
    Console::puts("\n");
}

void PageTable::free_page(unsigned long _page_no) {
    Console::puts("PageTable::free_page\n");
    unsigned long pde,pte,pageAdd,pageTableAdd;
    pageAdd=_page_no;

    pageAdd=(1023<<22)+(1023<<12);
    unsigned long* data=(unsigned long*) pageAdd;

    Console::putui(data[0]);
    Console::putui(data[1]);
    Console::putui(data[2]);
    Console::putui(data[3]);
    Console::putui(data[4]);
    Console::putui(data[1022]);


    // pde=pageAdd>>10;
    // pte=pageAdd<<10;
    // pte=pte>>10;

    // pageTableAdd=(1023<<22)+(pde<<12)+(pte<<2);
    // Console::putui(_page_no);
    // Console::putui(pageTableAdd);
    // Console::putui(pde);
    // Console::putui(pte);
    
    // unsigned long* metaData=(unsigned long*) (pageTableAdd);
    // Console::puts("metadata===");
    // Console::putui(metaData[0]);
    


// release the page from the pde and pte located at the 4 GB

    load();
}

// unsigned long PageTable::pdeFinder(unsigned long byteAddress){

// }