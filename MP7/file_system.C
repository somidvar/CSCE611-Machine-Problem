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
    inodes = new Inode[64];
}

FileSystem::~FileSystem() {
    Console::puts("unmounting file system\n");
    Save();
    /* Make sure that the inode list and the free list are saved. */
    assert(false);
}

/*--------------------------------------------------------------------------*/
/* FILE SYSTEM FUNCTIONS */
/*--------------------------------------------------------------------------*/

bool FileSystem::Mount(SimpleDisk *_disk) {
    Console::puts("mounting file system from disk\n");

    disk=_disk;
    unsigned char* iNodeData = new unsigned char[64 * 8];
    disk->read(0, freeBlockBitmap);
    disk->read(1, iNodeData);
    iNodesSetter((unsigned long long*)iNodeData);
    return true;
}
bool FileSystem::Save() {
    disk->write(0, freeBlockBitmap);  // saving the freeBlockBitmap into block 0
    unsigned long long* iNodesData = new unsigned long long[64 * 8];
    unsigned char* toBeWritten = (unsigned char*)iNodesData;
    disk->write(1, toBeWritten);  // saving the iNodes data into block 1
}

bool FileSystem::Format(SimpleDisk* _disk, unsigned int _size) {  // static!
    Console::puts("formatting disk\n");
    unsigned char* temp = new unsigned char[SimpleDisk::BLOCK_SIZE];
    _disk->write(1, temp);  // resetting the inodeBitmap

    for(int i=2;i<SimpleDisk::BLOCK_SIZE;i++)//setting all but the first two blocks to available
        temp[i]=1;

    _disk->write(0, temp);  // resetting the freeBlockBitmap
    return true;
    /* Here you populate the disk with an initialized (probably empty) inode list
       and a free list. Make sure that blocks used for the inodes and for the free list
       are marked as used, otherwise they may get overwritten. */
}

Inode* FileSystem::LookupFile(int _file_id) {
    Console::puts("looking up file with id = ");
    Console::puti(_file_id);
    Console::puts("\n");
    /* Here you go through the inode list to find the file. */

    for (int i = 0; i < 64; i++) {
        Inode* node = &inodes[i];
        if (node->id == _file_id)
            return node;
    }
    return NULL;
}

bool FileSystem::CreateFile(int _file_id) {
    Console::puts("creating file with id:");
    Console::puti(_file_id);
    Console::puts("\n");

    if (LookupFile(_file_id) != NULL)
        return false;

    int newDataBlock = getFreeBlock();
    Inode* newNode = getFreeInode();
    newNode->id = _file_id;
    newNode->reservedBits = 0;
    newNode->blockID = newDataBlock;
    newNode->fileSize = 1;
    return true;

    /* Here you check if the file exists already. If so, throw an error.
       Then get yourself a free inode and initialize all the data needed for the
       new file. After this function there will be a new file on disk. */
}

bool FileSystem::DeleteFile(int _file_id) {
    Console::puts("deleting file with id:");
    Console::puti(_file_id);
    Console::puts("\n");

    Inode* deleteINode = LookupFile(_file_id);
    if (deleteINode == NULL)
        return false;
    releaseBlock(deleteINode->blockID);
    releaseInode(deleteINode);
    return true;

    /* First, check if the file exists. If not, throw an error.
       Then free all blocks that belong to the file and delete/invalidate
       (depending on your implementation of the inode list) the inode. */
}

int FileSystem::getFreeBlock() {
    for (int i = 0; i < SimpleDisk::BLOCK_SIZE; i++) {
        if (freeBlockBitmap[i] == 1) {
            freeBlockBitmap[i] = 0;
            return i;
        }
    }
    Console::puts("MAYDAY at getFreeBlcok, there is no more block available\n");
    assert(false);
}
Inode* FileSystem::getFreeInode() {
    Inode* node;
    for (int i = 0; i < 64; i++) {
        node = &inodes[i];
        if (node->id == 0) {
            inodeCounter++;
            return node;
        }
    }
    Console::puts("Mayday in getFreeInode. There is no more iNode avaialble\n");
    assert(false);
}

void FileSystem::iNodesGetter(unsigned long long* _iNodesData) {
    int shifter;
    unsigned long long temp, data;
    Inode* node;
    for (int i = 0; i < 64; i++) {
        node = &inodes[i];
        shifter = 0;
        temp = 0;
        data = 0;

        temp = node->id;
        data += (temp << shifter);  // bit 0 to 16 is the blockName

        shifter += 16;
        temp = node->blockID;
        data += (temp << shifter);  // bit 16 to 31 is the blockID

        shifter += 16;
        temp = node->reservedBits;
        data += (temp << shifter);  // bit 32 to 47 is the reservedBits

        shifter += 16;
        temp = node->fileSize;
        data += (temp << shifter);  // bit 48 to 64 is the fileSize

        _iNodesData[i] = data;
    }
}
void FileSystem::iNodesSetter(unsigned long long* _iNodesData) {
    unsigned long long temp, data;
    Inode* node;

    for (int i = 0; i < 64; i++) {
        data = _iNodesData[i];
        node = &inodes[i];

        temp = data;  // 48 to 64
        temp = temp << 0;
        temp = temp >> 0;
        temp = temp >> 48;
        node->fileSize = temp;

        temp = data;  // 32 to 48
        temp = temp << 16;
        temp = temp >> 16;
        temp = temp >> 32;
        node->reservedBits = temp;

        temp = data;  // 16 to 32
        temp = temp << 32;
        temp = temp >> 32;
        temp = temp >> 16;
        node->blockID = temp;

        temp = data;  // 0 to 16
        temp = temp << 48;
        temp = temp >> 48;
        temp = temp >> 0;
        node->id = temp;
    }
}
void FileSystem::releaseInode(Inode* _deleteINode) {
    _deleteINode->blockID = 0;
    _deleteINode->reservedBits = 0;
    _deleteINode->fileSize = 0;
    _deleteINode->id = 0;
    inodeCounter--;
}
void FileSystem::releaseBlock(int _blockNum) {
    freeBlockBitmap[_blockNum] = 1;
}
SimpleDisk* FileSystem::diskgetter(){
    return disk;
}