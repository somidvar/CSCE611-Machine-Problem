/*
     File        : file_system.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File System class.
                   Has support for numerical file identifiers.
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
#include "file_system.H"

/*--------------------------------------------------------------------------*/
/* CLASS Inode */
/*--------------------------------------------------------------------------*/

/* You may need to add a few functions, for example to help read and store
   inodes from and to disk. */

/*--------------------------------------------------------------------------*/
/* CLASS FileSystem */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

FileSystem::FileSystem() {
    Console::puts("In FileSystem constructor.\n");

    assert(false);
}

FileSystem::~FileSystem() {
    Console::puts("unmounting file system\n");
    /* Make sure that the inode list and the free list are saved. */
    assert(false);
}

/*--------------------------------------------------------------------------*/
/* FILE SYSTEM FUNCTIONS */
/*--------------------------------------------------------------------------*/

bool FileSystem::Mount(SimpleDisk* _disk) {
    Console::puts("mounting file system from disk\n");
    SimpleDisk* currentDisk = _disk;

    unsigned char* inodesBitmap;
    currentDisk->read(0, inodesBitmap);
    currentDisk->read(1, freeBlockBitmap);

    assert(false);
}

bool FileSystem::Format(SimpleDisk* _disk, unsigned int _size) {  // static!
    Console::puts("formatting disk\n");
    SimpleDisk* currentDisk = _disk;
    unsigned char* temp = new unsigned char[SimpleDisk::BLOCK_SIZE];
    currentDisk->write(0, temp);  // resetting the inodeBitmap

    temp[0] = 1;
    temp[1] = 1;
    currentDisk->write(1, temp);  // resetting the freeBlockBitmap

    /* Here you populate the disk with an initialized (probably empty) inode list
       and a free list. Make sure that blocks used for the inodes and for the free list
       are marked as used, otherwise they may get overwritten. */

    assert(false);
}

Inode* FileSystem::LookupFile(int _file_id) {
    Console::puts("looking up file with id = ");
    Console::puti(_file_id);
    Console::puts("\n");
    /* Here you go through the inode list to find the file. */
    assert(false);
}

bool FileSystem::CreateFile(int _file_id) {
    Console::puts("creating file with id:");
    Console::puti(_file_id);
    Console::puts("\n");

    if(LookupFile(_file_id)==NULL)
        return false;

    int newInode=getFreeBlock();
    int newDataBlock=getFreeBlock();
    inodesBitmap[newDataBlock]=newInode; //the metadata of this block is stored at newInode 
    inodesBitmap[newInode]=-1;//this block does not have any INode
    Inode *myInode=new Inode();
    myInode->id=_file_id;
    myInode->cursorPos=0;
    myInode->blockID=newDataBlock;
    myInode->fileSize=1;
    return true;
    


    /* Here you check if the file exists already. If so, throw an error.
       Then get yourself a free inode and initialize all the data needed for the
       new file. After this function there will be a new file on disk. */
    assert(false);
}

bool FileSystem::DeleteFile(int _file_id) {
    Console::puts("deleting file with id:");
    Console::puti(_file_id);
    Console::puts("\n");
    /* First, check if the file exists. If not, throw an error.
       Then free all blocks that belong to the file and delete/invalidate
       (depending on your implementation of the inode list) the inode. */
}

int FileSystem::getFreeBlock() {
    for (int i = 0; i < SimpleDisk::BLOCK_SIZE; i++) {
        if (freeBlockBitmap[i] == 0) {
            freeBlockBitmap[i] = 1;
            return i;
        }
    }
    return -1;
}
int FileSystem::getFreeInode() {
    // for(int i=0;i<SimpleDisk::BLOCK_SIZE;i++){
    //     if(freeBlockBitmap[i]==0){
    //         freeBlockBitmap[i]=1;
    //         return i;
    //     }
    // }
    // return -1;
}

void Inode::iNodeSerializer(Inode* _iNode,unsigned long &_data){
    int shifter=0;
    _data=0;
    _data+=((_iNode->id)<<shifter);//bit 0 to 16 is the blockName
    shifter+=16;
    _data+=((_iNode->blockID)<<shifter);//bit 16 to 31 is the blockID
    shifter+=16;
    _data+=((_iNode->cursorPos)<<shifter);//bit 32 to 47 is the cursorPos
    shifter+=16;
    _data+=((_iNode->fileSize)<<shifter);//bit 48 to 64 is the fileSize

}
void Inode::iNodeDeserializer(unsigned long data,unsigned short &id, unsigned short &blockID,unsigned short &cursorPos,unsigned short &fileSize){
    unsigned long temp;

    temp=data;//48 to 64
    temp=temp<<0;
    temp=temp>>0;
    temp=temp>>48;
    fileSize=temp;

    temp=data;//32 to 48
    temp=temp<<16;
    temp=temp>>16;
    temp=temp>>32;
    cursorPos=temp;

    temp=data;//16 to 32
    temp=temp<<32;
    temp=temp>>32;
    temp=temp>>16;
    blockID=temp;
    
    temp=data;//0 to 16
    temp=temp<<48;
    temp=temp>>48;
    temp=temp>>0;
    id=temp;    
}