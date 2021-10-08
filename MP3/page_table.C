#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"

PageTable *PageTable::current_page_table = NULL;
unsigned int PageTable::paging_enabled = 0;
ContFramePool *PageTable::kernel_mem_pool = NULL;
ContFramePool *PageTable::process_mem_pool = NULL;
unsigned long PageTable::shared_size = 0;

void PageTable::init_paging(ContFramePool *_kernel_mem_pool,
                            ContFramePool *_process_mem_pool,
                            const unsigned long _shared_size)
{
   Console::puts("init_paging is started\n");

   kernel_mem_pool = _kernel_mem_pool;
   process_mem_pool = _process_mem_pool;
   shared_size = _shared_size;

   Console::puts("init_paging is ended\n");
}

PageTable::PageTable()
{
   Console::puts("Entering the constructor\n");
   //for meta data the important first 12 bits are present, read/write, supervisor/user, reserved, reserved, accessed, dirty, reserved, reserved, available and available from begining to end
   if (!page_directory)
   {                                                                //the page directory does not exist
      unsigned long directoryAdd = kernel_mem_pool->get_frames(1);  //getting a frame from the kernel to store the directory
      page_directory = (unsigned long *)(directoryAdd * PAGE_SIZE); //pointing to the location of the new frame for storing the directory
      unsigned long metaData = NULL;
      metaData = 1*0 + 2 * 1 + 4 * 0; //not present,read/write and supervisor mode
      for (int i = 0; i < 1024; i++)
      {
         page_directory[i] = metaData; //initializing the directory entries
      }
      //mapping the first 4 MB
      unsigned long directMappingAdd = kernel_mem_pool->get_frames(1); //getting a frame from the kernel to store the directory
      directMapping = (unsigned long *)(directMappingAdd * PAGE_SIZE); //pointing to the location of the new frame for storing the directory
      metaData = 1 * 1 + 2 * 1 + 4 * 0;                                //present, read/write and supervisor
      for (unsigned long i = 0; i < 1024; i++)
      {
         directMapping[i] = (i * PAGE_SIZE) + metaData;
      }
      metaData = 1 * 1 + 2 * 1 + 4 * 0;                            //present, read/write and supervisor
      page_directory[0] = (directMappingAdd*PAGE_SIZE) + metaData; //adding the address of the 4MB mapping as the first entry of the page directory
      Console::puts("Directory is created\n");   
   }
   Console::puts("constructor is ended\n");
}

void PageTable::load()
{
   Console::puts("Entering load\n");
   current_page_table = this;
   write_cr3((unsigned long)page_directory);
   Console::puts("Loaded page table\n");
}

void PageTable::enable_paging()
{
   Console::puts("Entering enable_paging\n");
   write_cr0(read_cr0() | 0x80000000);
   Console::puts("Enabled paging\n");
}

void PageTable::handle_fault(REGS *_r)
{
   Console::puts("Entering handle_fault\n");
   unsigned long faultCode=_r->err_code;
   if (faultCode & 1 !=1)//making sure that we are dealing with a page fault
   {
      Console::puts("MAYDAY at handle_fault, the fault code is not equal to not present\n");
      assert(false);
   }
   unsigned long newAdd = (unsigned long)read_cr2();//reading the address issued by CPU
   unsigned long directoryBits = newAdd >> 22;//getting the first 10 bits
   unsigned long pageBits = newAdd << 10; //removing the directory entry
   pageBits = pageBits >> 22;//getting the second 10 bits

   unsigned long directoryMetaData = 0;
   unsigned long pageMetaData = 0;
   directoryMetaData = current_page_table->page_directory[directoryBits] & 0x00000001; //if the directory has a record of the page
   if (directoryMetaData == 0) //the directory entry is not present
   {
      unsigned long newPageAdd=kernel_mem_pool->get_frames(1);
      current_page_table->page_directory[directoryBits]=newPageAdd*PAGE_SIZE + (1*1+2*1+4*0);//present, read/write, kernel
      unsigned long * newPage=(unsigned long*)(newPageAdd*PAGE_SIZE);
      for(int i=0;i<1024;i++){
         newPage[i]=(0*1+2*1+4*0);//not present, read/write, kernel
      }
   }
   unsigned long directoryEntry=current_page_table->page_directory[directoryBits];
   directoryEntry=directoryEntry>>12;//removing the meta data
   unsigned long* pageTablePage=(unsigned long*)(directoryEntry*PAGE_SIZE);
   pageMetaData = pageTablePage[pageBits] & 0x00000001; //if the page is present at the page table page
   if (pageMetaData == 0) //the directory entry is present but the page is not
   {
      unsigned long newFrameAdd=process_mem_pool->get_frames(1); //this time we get a frame from the prcoess to sit the actual data
      pageTablePage[pageBits]=newFrameAdd*PAGE_SIZE + (1*1+2*1+4*0);//present, read/write, kernel
   }
   else//the page is present at the page table page
   {
      Console::puts("MAYDAY at handle_fault\n");
      assert(false);
   }
   Console::puts("handled page fault\n");
}
