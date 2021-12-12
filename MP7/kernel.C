/*
    File: kernel.C

    Author: R. Bettati
            Department of Computer Science
            Texas A&M University
    Date  : 2021/11/28


    This file has the main entry point to the operating system.

    MAIN FILE FOR MACHINE PROBLEM "FILE SYSTEM"

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#define MB *(0x1 << 20)
#define KB *(0x1 << 10)

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "exceptions.H"
#include "file.H"
#include "file_system.H" /* FILE SYSTEM */
#include "frame_pool.H"  /* MEMORY MANAGEMENT */
#include "gdt.H"
#include "idt.H" /* EXCEPTION MGMT.   */
#include "interrupts.H"
#include "irq.H"
#include "machine.H" /* LOW-LEVEL STUFF   */
#include "mem_pool.H"
#include "simple_disk.H"  /* DISK DEVICE */
#include "simple_timer.H" /* TIMER MANAGEMENT  */

/*--------------------------------------------------------------------------*/
/* MEMORY MANAGEMENT */
/*--------------------------------------------------------------------------*/

/* -- A POOL OF FRAMES FOR THE SYSTEM TO USE */
FramePool *SYSTEM_FRAME_POOL;

/* -- A POOL OF CONTIGUOUS MEMORY FOR THE SYSTEM TO USE */
MemPool *MEMORY_POOL;

typedef long unsigned int size_t;

// replace the operator "new"
void *operator new(size_t size) {
    unsigned long a = MEMORY_POOL->allocate((unsigned long)size);
    return (void *)a;
}

// replace the operator "new[]"
void *operator new[](size_t size) {
    unsigned long a = MEMORY_POOL->allocate((unsigned long)size);
    return (void *)a;
}

// replace the operator "delete"
void operator delete(void *p, size_t s) {
    MEMORY_POOL->release((unsigned long)p);
}

// replace the operator "delete[]"
void operator delete[](void *p) {
    MEMORY_POOL->release((unsigned long)p);
}

/*--------------------------------------------------------------------------*/
/* DISK */
/*--------------------------------------------------------------------------*/

/* -- A POINTER TO THE SYSTEM DISK */
SimpleDisk *SYSTEM_DISK;

#define SYSTEM_DISK_SIZE (10 MB)

/*--------------------------------------------------------------------------*/
/* FILE SYSTEM */
/*--------------------------------------------------------------------------*/

/* -- A POINTER TO THE SYSTEM FILE SYSTEM */
FileSystem *FILE_SYSTEM;

/*--------------------------------------------------------------------------*/
/* CODE TO EXERCISE THE FILE SYSTEM */
/*--------------------------------------------------------------------------*/

void exercise_file_system(FileSystem *_file_system) {
    char *STRING1 = new char[512];
    char *STRING2 = new char[512];

    char *combined1 = new char[1024];
    char *combined2 = new char[1024];
    for (int i = 0; i < 512; i++) {
        STRING1[i] = i % 128;
        STRING1[i] = 3;
    }

    /* -- Create two files -- */

    assert(_file_system->CreateFile(1));
    assert(_file_system->CreateFile(2));

    /* -- "Open" the two files -- */

    {
        File file1(_file_system, 1);

        File file2(_file_system, 2);

        /* -- Write into File 1 -- */
        file1.Write(14, STRING1);
        file1.Write(200, STRING1);
        file1.Write(400, STRING1);
        file1.Write(10, STRING2);
        file1.Write(400, STRING1);

        for (int i = 0; i < 14; i++)
            combined1[i] = STRING1[i];
        for (int i = 0; i < 200; i++)
            combined1[i + 14] = STRING1[i];
        for (int i = 0; i < 400; i++)
            combined1[i + 200 + 14] = STRING1[i];
        for (int i = 0; i < 10; i++)
            combined1[i + 400 + 200 + 14] = STRING2[i];
        for (int i = 0; i < 400; i++)
            combined1[i + 10 + 400 + 200 + 14] = STRING1[i];
        /* -- Write into File 2 -- */

        file2.Write(100, STRING1);
        file2.Write(200, STRING2);
        file2.Write(300, STRING2);
        file2.Write(400, STRING1);
        file2.Write(24, STRING1);

        for (int i = 0; i < 100; i++)
            combined2[i] = STRING1[i];
        for (int i = 0; i < 200; i++)
            combined2[i + 100] = STRING2[i];
        for (int i = 0; i < 300; i++)
            combined2[i + 200 + 100] = STRING2[i];
        for (int i = 0; i < 400; i++)
            combined2[i + 300 + 200 + 100] = STRING1[i];
        for (int i = 0; i < 24; i++)
            combined2[i + 400 + 300 + 200 + 100] = STRING1[i];

        /* -- Files will get automatically closed when we leave scope  -- */
    }

    {
        /* -- "Open files again -- */
        File file1(_file_system, 1);
        File file2(_file_system, 2);

        /* -- Read from File 1 and check result -- */
        file1.Reset();
        assert(file1.EoF()==false);
        char *result1 = new char[1024];
        assert(file1.Read(1024, result1) == 1024);
        for (int i = 0; i < 1024; i++) {
            // Console::putui(result1[i]);
            // Console::putui(combined1[i]);
            // Console::puts("\n");
            assert(result1[i] == combined1[i]);
        }
        assert(file1.EoF()==true);
        

        /* -- Read from File 2 and check result -- */
        file2.Reset();
        assert(file2.EoF()==false);
        char *result2 = new char[1024];
        assert(file2.Read(1024, result2) == 1024);
        for (int i = 0; i < 1024; i++) {
            assert(result2[i] == combined2[i]);
        }
        assert(file2.EoF()==true);

        /* -- "Close" files again -- */
    }

    /* -- Delete both files -- */
    assert(_file_system->DeleteFile(1));
    assert(_file_system->LookupFile(1)==NULL);
    assert(_file_system->DeleteFile(2));
    assert(_file_system->LookupFile(2)==NULL);
}

/*--------------------------------------------------------------------------*/
/* MAIN ENTRY INTO THE OS */
/*--------------------------------------------------------------------------*/

int main() {
    GDT::init();
    Console::init();
    IDT::init();
    ExceptionHandler::init_dispatcher();
    IRQ::init();
    InterruptHandler::init_dispatcher();

    Console::output_redirection(true);

    /* -- EXAMPLE OF AN EXCEPTION HANDLER -- */

    class DBZ_Handler : public ExceptionHandler {
       public:
        virtual void handle_exception(REGS *_regs) {
            Console::puts("DIVISION BY ZERO!\n");
            for (;;)
                ;
        }
    } dbz_handler;

    ExceptionHandler::register_handler(0, &dbz_handler);

    /* -- INITIALIZE MEMORY -- */
    /*    NOTE: We don't have paging enabled in this MP. */
    /*    NOTE2: This is not an exercise in memory management. The implementation
                of the memory management is accordingly *very* primitive! */

    /* ---- Initialize a frame pool; details are in its implementation */
    FramePool system_frame_pool;
    SYSTEM_FRAME_POOL = &system_frame_pool;

    /* ---- Create a memory pool of 256 frames. */
    MemPool memory_pool(SYSTEM_FRAME_POOL, 256);
    MEMORY_POOL = &memory_pool;

    /* -- MEMORY ALLOCATOR SET UP. WE CAN NOW USE NEW/DELETE! -- */

    /* -- INITIALIZE THE TIMER (we use a very simple timer).-- */

    /* Question: Why do we want a timer? We have it to make sure that
                 we enable interrupts correctly. If we forget to do it,
                 the timer "dies". */

    SimpleTimer timer(100); /* timer ticks every 10ms. */
    InterruptHandler::register_handler(0, &timer);
    /* The Timer is implemented as an interrupt handler. */

    /* -- DISK DEVICE -- */

    SYSTEM_DISK = new SimpleDisk(DISK_ID::MASTER, SYSTEM_DISK_SIZE);

    class Disk_Silencer : public InterruptHandler {
       public:
        virtual void handle_interrupt(REGS *_regs) {
            // we do nothing here. Just consume the interrupt
        }
    } disk_silencer;

    InterruptHandler::register_handler(14, &disk_silencer);

    /* -- FILE SYSTEM -- */

    FILE_SYSTEM = new FileSystem();

    /* NOTE: The timer chip starts periodically firing as
             soon as we enable interrupts.
             It is important to install a timer handler, as we
             would get a lot of uncaptured interrupts otherwise. */

    /* -- ENABLE INTERRUPTS -- */

    Machine::enable_interrupts();

    /* -- MOST OF WHAT WE NEED IS SETUP. THE KERNEL CAN START. */

    Console::puts("Hello World!\n");

    /* -- HERE WE STRESS TEST THE FILE SYSTEM -- */

    assert(FileSystem::Format(SYSTEM_DISK, (128 KB)));  // Don't try this at home!
    /* This is a really small file system. This allows you to use a very crude
       implementation for the free block list. */

    assert(FILE_SYSTEM->Mount(SYSTEM_DISK));  // 'connect' disk to file system.

    for (int j = 0;; j++) {
        exercise_file_system(FILE_SYSTEM);
    }

    /* -- AND ALL THE REST SHOULD FOLLOW ... */

    assert(false); /* WE SHOULD NEVER REACH THIS POINT. */

    /* -- WE DO THE FOLLOWING TO KEEP THE COMPILER HAPPY. */
    return 1;
}
