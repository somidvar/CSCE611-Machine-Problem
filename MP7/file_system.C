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
    inodes = new Inode[Inode::MAX_INODE];
    freeBlockBitmap = new unsigned char[SimpleDisk::BLOCK_SIZE];
}

FileSystem::~FileSystem() {
    Console::puts("unmounting file system\n");
    Save();
    /* Make sure that the inode list and the free list are saved. */
}

/*--------------------------------------------------------------------------*/
/* FILE SYSTEM FUNCTIONS */
/*--------------------------------------------------------------------------*/

bool FileSystem::Mount(SimpleDisk* _disk) {
    Console::puts("mounting file system from disk\n");

    disk = _disk;
    disk->read(0, freeBlockBitmap);  // loading the freeBlockBitmap

    unsigned long long* iNodeData = new unsigned long long[Inode::MAX_INODE];  // each inode is 64 bit=8 byte
    unsigned long long* temp = new unsigned long long[SimpleDisk::BLOCK_SIZE / 8];
    for (int i = 1; i < Inode::INODE_BLOCK_NUMBER + 1; i++) {  // loading inodes
        disk->read(i, (unsigned char*)temp);                   // illegal type-cast
        for (int j = 0; j < SimpleDisk::BLOCK_SIZE / 8; j++) {
            iNodeData[(i - 1) * Inode::INODE_PER_BLOCK + j] = temp[j];
        }
    }
    iNodesSetter(iNodeData);

    delete[] temp;
    delete[] iNodeData;
    return true;
}
bool FileSystem::Save() {
    Console::puts("Save function\n");
    disk->write(0, freeBlockBitmap);  // saving the freeBlockBitmap into block 0

    unsigned long long* iNodeData = new unsigned long long[Inode::MAX_INODE];
    unsigned long long* temp = new unsigned long long[SimpleDisk::BLOCK_SIZE / 8];
    iNodesGetter(iNodeData);
    for (int i = 1; i < Inode::INODE_BLOCK_NUMBER + 1; i++) {  // saving the inode into the blocks starting at 1
        for (int j = 0; j < SimpleDisk::BLOCK_SIZE / 8; j++) {
            temp[j] = iNodeData[(i - 1) * Inode::INODE_PER_BLOCK + j];
        }
        disk->write(i, (unsigned char*)temp);  // illegal type-cast
    }
    delete[] temp;
    delete[] iNodeData;
    return true;
}

void FileSystem::tester() {
    /*
    Mount(disk);
    int myBlockNum = 10;
    inodes[myBlockNum].id = 10;
    inodes[myBlockNum].blockID = 20;
    inodes[myBlockNum].nextINode = 30;
    inodes[myBlockNum].fileSize = 40;

    Save();
    Format(disk, 1024);
    Mount(disk);
    inodes[myBlockNum].nextINode = 50;
    inodes[myBlockNum].fileSize = 60;
    Save();
    Mount(disk);
    unsigned char* hooshang = new unsigned char[1024];
    disk->read(1, hooshang);
    int solat;
    solat = hooshang[myBlockNum * 8 + 0];
    Console::putui(solat);
    solat = hooshang[myBlockNum * 8 + 1];
    Console::putui(solat);
    solat = hooshang[myBlockNum * 8 + 2];
    Console::putui(solat);
    solat = hooshang[myBlockNum * 8 + 3];
    Console::putui(solat);
    solat = hooshang[myBlockNum * 8 + 4];
    Console::putui(solat);
    solat = hooshang[myBlockNum * 8 + 5];
    Console::putui(solat);
    solat = hooshang[myBlockNum * 8 + 6];
    Console::putui(solat);
    solat = hooshang[myBlockNum * 8 + 7];
    Console::putui(solat);
    solat = hooshang[myBlockNum * 8 + 8];
    Console::putui(solat);

    inodes[myBlockNum].id = 11;
    inodes[myBlockNum].blockID = 21;
    inodes[myBlockNum].nextINode = 31;
    inodes[myBlockNum].fileSize = 41;

    Mount(disk);
    Console::puts("---------------------------------\n");
    int mamad;
    mamad = inodes[myBlockNum].id;
    Console::putui(mamad);
    mamad = inodes[myBlockNum].blockID;
    Console::putui(mamad);
    mamad = inodes[myBlockNum].nextINode;
    Console::putui(mamad);
    mamad = inodes[myBlockNum].fileSize;
    Console::putui(mamad);
    assert(false);
    */
}

bool FileSystem::Format(SimpleDisk* _disk, unsigned int _size) {
    Console::puts("formatting disk\n");

    unsigned char* temp = new unsigned char[SimpleDisk::BLOCK_SIZE];
    for (int i = 0; i < SimpleDisk::BLOCK_SIZE; i++) {  // setting all blocks to available
        temp[i] = 255;
    }
    bitmapSetter(temp, 0, 0);                                // the first block is reserved for the freeBlockBitmap
    for (int i = 1; i < Inode::INODE_BLOCK_NUMBER + 1; i++)  // setting the inode blocks to unavailable
        bitmapSetter(temp, i, 0);
    _disk->write(0, temp);  // resetting the freeBlockBitmap

    for (int i = 0; i < SimpleDisk::BLOCK_SIZE; i++)  // setting temp to 0
        temp[i] = 0;
    for (int i = 1; i < Inode::INODE_BLOCK_NUMBER + 1; i++) {
        _disk->write(i, temp);  // resetting the inodes data blocks
    }

    delete[] temp;
    return true;
}

bool FileSystem::CreateFile(int _file_id, int _fileSize) {
    Console::puts("creating file with id:");
    Console::puti(_file_id);
    Console::puts("\n");

    if (LookupFile(_file_id) != NULL) {
        Console::puts("The file has been created before\n");
        return false;
    }

    if (_fileSize > 1024 * 64) {
        Console::puts("MAYDAY at CreateFile. The file is bigger than 64 KB\n");
        assert(false);
    }

    unsigned char* temp = new unsigned char[SimpleDisk::BLOCK_SIZE];
    Inode *next, *node;
    int newDataBlock, fileBlockNum;
    node = getFreeInode();
    if (_fileSize % SimpleDisk::BLOCK_SIZE == 0)  // rounding the fileSize to blockSize
        fileBlockNum = _fileSize / SimpleDisk::BLOCK_SIZE;
    else
        fileBlockNum = _fileSize / SimpleDisk::BLOCK_SIZE + 1;
    for (int i = 0; i < fileBlockNum; i++) {
        newDataBlock = getFreeBlock();
        node->id = _file_id;
        node->blockID = newDataBlock;
        node->fileSize = 0;
        if (i < fileBlockNum - 1) {  // the last node does not get a next inode
            next = getFreeInode();
            node->nextINode = next->iNodeNumber;
        } else
            node->nextINode = 0;
        disk->write(newDataBlock, temp);  // cleaning the file
        node = next;
    }
    // Console::puts("------------------\n");
    // Inode* mamad = LookupFile(_file_id);
    // while (mamad->blockID != 0) {
    //     Console::putui(mamad->id);
    //     Console::putui(mamad->blockID);
    //     Console::putui(mamad->nextINode);
    //     Console::putui(mamad->iNodeNumber);
    //     if (mamad->nextINode != 0)
    //         mamad = &inodes[mamad->nextINode];
    //     else
    //         break;
    //     Console::puts("******************\n");
    // }
    // Console::puts("===================\n");


    delete[] temp;
    return true;
}
Inode* FileSystem::LookupFile(int _file_id) {
    Console::puts("looking up file with id = ");
    Console::puti(_file_id);
    Console::puts("\n");

    Inode* node;
    node = FileLastINode(_file_id);
    if (node == NULL)  // file does not exist
        return NULL;
    while (iNodeParentFinder(node) != NULL) {
        node = iNodeParentFinder(node);
    }
    return node;  // this is the first part of the file
}
Inode* FileSystem::iNodeParentFinder(Inode* _inode) {
    Console::puts("iNodeParentFinder\n");

    Inode* currentNode = _inode;
    Inode* candid;
    for (int i = 0; i < Inode::MAX_INODE; i++) {
        candid = &inodes[i];

        if (candid->id == currentNode->id &&
            candid->nextINode == currentNode->iNodeNumber &&
            candid->nextINode != 0) {
            return candid;
        }
    }
    return NULL;
}
Inode* FileSystem::FileLastINode(int _file_id) {
    Inode* node;
    for (int i = 0; i < Inode::MAX_INODE; i++) {
        node = &inodes[i];
        if (node->id == _file_id && node->nextINode == 0 && node->blockID != 0) {
            return node;
        }
    }
    return NULL;
}

bool FileSystem::DeleteFile(int _file_id) {
    Console::puts("deleting file with id:");
    Console::puti(_file_id);
    Console::puts("\n");

    if (LookupFile(_file_id) == NULL)  // the file does not exist
        return false;
    Inode* deleteNode;
    for (int i = 0; i < Inode::MAX_INODE; i++) {
        deleteNode = &inodes[i];
        if (deleteNode->id == _file_id && deleteNode->blockID != 0) {
            releaseBlock(deleteNode->blockID);
            releaseInode(deleteNode);
        }
    }
    return true;

    /* First, check if the file exists. If not, throw an error.
       Then free all blocks that belong to the file and delete/invalidate
       (depending on your implementation of the inode list) the inode. */
}

int FileSystem::getFreeBlock() {
    int blockStat;
    for (int i = 0; i < Inode::MAX_INODE; i++) {  // we have block number equal to inodes
        blockStat = bitmapGetter(freeBlockBitmap, i);
        if (bitmapGetter(freeBlockBitmap, i) == 1) {
            bitmapSetter(freeBlockBitmap, i, 0);
            return i;
        }
    }
    Console::puts("MAYDAY at getFreeBlcok, there is no more block available\n");
    assert(false);
}
Inode* FileSystem::getFreeInode() {
    Inode* node;
    for (int i = 0; i < Inode::MAX_INODE; i++) {
        node = &inodes[i];
        if (node->blockID == 0) {
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
    for (int i = 0; i < Inode::MAX_INODE; i++) {
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
        temp = node->nextINode;
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

    for (int i = 0; i < Inode::MAX_INODE; i++) {
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
        node->nextINode = temp;

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

        node->iNodeNumber = i;  // setting the index of the INode. We don't save this as it is easy to retrive it
    }
}
void FileSystem::releaseInode(Inode* _deleteINode) {
    _deleteINode->blockID = 0;
    _deleteINode->nextINode = 0;
    _deleteINode->fileSize = 0;
    _deleteINode->id = 0;
    inodeCounter--;
}
void FileSystem::releaseBlock(int _blockNum) {
    bitmapSetter(freeBlockBitmap, _blockNum, 1);
}
SimpleDisk* FileSystem::diskgetter() {
    return disk;
}
int FileSystem::bitmapGetter(unsigned char* _char, int _blockNum) {
    int charBlockNum = _blockNum / 8;  // char number of _blockNum
    int blockInChar = _blockNum % 8;   // index number of _blockNum
    // For example if I want the block number 22--> I should look ay charBlockNum=2.
    // Inside the charBlockNum=2, I should look at the index 6
    unsigned char temp = _char[charBlockNum];

    if (blockInChar > 0) {
        temp = temp >> (blockInChar - 1);
        temp = temp << (blockInChar - 1);
    }
    temp = temp << (8 - blockInChar - 1);
    temp = temp >> (8 - blockInChar - 1);

    temp = temp >> blockInChar;
    return temp;
}
void FileSystem::bitmapSetter(unsigned char* _char, int _blockNum, int _free) {
    int charBlockNum = _blockNum / 8;  // char number of _blockNum
    int blockInChar = _blockNum % 8;   // index number of _blockNum
    unsigned char currentStat = _char[charBlockNum];
    unsigned char newStat = 0;
    for (int i = 0; i < 8; i++) {
        if (i == blockInChar) {
            newStat += _free << i;
        } else {
            newStat += (bitmapGetter(_char, charBlockNum * 8 + i)) << i;
        }
    }
    _char[charBlockNum] = newStat;
}
Inode* FileSystem::inodesInode(int _inodesIndex) {
    return &inodes[_inodesIndex];
}
