/*
    File: file_system.H

    Author: R. Bettati
            Department of Computer Science
            Texas A&M University
    Date  : 21/11/28

    Description: Simple File System.


*/

#ifndef _FILE_SYSTEM_H_  // include file only once
#define _FILE_SYSTEM_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "simple_disk.H"

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

class FileSystem;

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

class Inode {
    friend class FileSystem;  // The inode is in an uncomfortable position between
    friend class File;        // File System and File. We give both full access
                              // to the Inode.

   private:
    unsigned short id;            // File "name"
    unsigned short blockID;       // the block where the data sits
    unsigned short reservedBits;  // position of the cursor when writting or reading
    unsigned short fileSize;      // file size in blocks

    /* You will need additional information in the inode, such as allocation
       information. */

    FileSystem *fs;  // It may be handy to have a pointer to the File system.
                     // For example when you need a new block or when you want
                     // to load or save the inode list. (Depends on your
                     // implementation.)

    /* You may need a few additional functions to help read and store the
       inodes from and to disk. */
};

/*--------------------------------------------------------------------------*/
/* FORWARD DECLARATIONS */
/*--------------------------------------------------------------------------*/

/* (none) */

/*--------------------------------------------------------------------------*/
/* F i l e S y s t e m  */
/*--------------------------------------------------------------------------*/

class FileSystem {
    friend class Inode;

   private:
    /* -- DEFINE YOUR FILE SYSTEM DATA STRUCTURES HERE. */

    SimpleDisk *disk;
    unsigned int size;

    static constexpr unsigned int MAX_INODES = SimpleDisk::BLOCK_SIZE / sizeof(Inode);
    /* Just as an example, you can store MAX_INODES in a single INODES block */

    Inode *inodes;                                       // the inode list
                                                         /* The inode list */
    void iNodesGetter(unsigned long long *_iNodesData);  // converting iNodes to a unsigned long long* which is 8*64 Byte
    void iNodesSetter(unsigned long long *_iNodesData);  // converting a 8*64 Byte value into iNodes
    unsigned short inodeCounter;                         // number of free iNodes

    unsigned char *freeBlockBitmap;  // bitmap of the disk blocks and inodes where 0 means free and 1 is busy
    /* The free-block list. You may want to implement the "list" as a bitmap.
       Feel free to use an unsigned char to represent whether a block is free or not;
       no need to go to bits if you don't want to.
       If you reserve one block to store the "free list", you can handle a file system up to
       256kB. (Large enough as a proof of concept.) */

    // short GetFreeInode();
    // int GetFreeBlock();
    /* It may be helpful to two functions to hand out free inodes in the inode list and free
       blocks. These functions also come useful to class Inode and File. */

    int getFreeBlock();
    Inode *getFreeInode();
    void releaseInode(Inode *deleteINode);
    void releaseBlock(int _blockNum);

   public:
    FileSystem();
    /* Just initializes local data structures. Does not connect to disk yet. */

    ~FileSystem();
    /* Unmount file system if it has been mounted. */

    bool Save();  // to save the filesystem into the disk

    bool Mount(SimpleDisk *_disk);
    /* Associates this file system with a disk. Limit to at most one file system per disk.
       Returns true if operation successful (i.e. there is indeed a file system on the disk.) */

    static bool Format(SimpleDisk *_disk, unsigned int _size);
    /* Wipes any file system from the disk and installs an empty file system of given size. */

    Inode *LookupFile(int _file_id);
    /* Find file with given id in file system. If found, return its inode.
         Otherwise, return null. */

    bool CreateFile(int _file_id);
    /* Create file with given id in the file system. If file exists already,
       abort and return false. Otherwise, return true. */

    bool DeleteFile(int _file_id);
    /* Delete file with given id in the file system; free any disk block occupied by the file. */

    SimpleDisk* diskgetter();
};
#endif
