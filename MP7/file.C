/*
     File        : file.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File class, with support for
                   sequential read/write operations.
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
#include "file.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR/DESTRUCTOR */
/*--------------------------------------------------------------------------*/

File::File(FileSystem *_fs, int _id) {
    Console::puts("Opening file.\n");

    currentFileSystem = _fs;
    currentFileID = _id;

    fileINode = currentFileSystem->LookupFile(currentFileID);  // getting the inode of the first part of the file
    if (fileINode == NULL) {
        Console::puts("The file should have been created but it cannot be found\n");
        assert(false);
    }

    currentDisk = currentFileSystem->diskgetter();
    cursorPos = 0;

    currentDisk->read(fileINode->blockID, block_cache);
}

File::~File() {
    Console::puts("Closing file.\n");
    /* Make sure that you write any cached data to disk. */
    /* Also make sure that the inode in the inode list is updated. */
}

/*--------------------------------------------------------------------------*/
/* FILE FUNCTIONS */
/*--------------------------------------------------------------------------*/

int File::Read(unsigned int _n, char *_buf) {
    Console::puts("reading from file\n");

    if (_n + cursorPos > 64 * 1024) {
        Console::puts("MAYDAY at File Read. Out of range\n");
        assert(false);
    }

    int counter;
    int blockChar = cursorPos % SimpleDisk::BLOCK_SIZE;
    char *blockContent = new char[SimpleDisk::BLOCK_SIZE];
    currentDisk->read(fileINode->blockID, (unsigned char *)blockContent);  // ILLELGAL TYPE-CAST
    for (counter = 0; counter < _n; counter++) {
        // if (blockChar > fileINode->fileSizeChar) {
        //     Console::puts("MAYDAY at Read File. Requested char number is bigger than the file size\n");
        //     assert(false);
        // }
        _buf[counter] = blockContent[blockChar];
        blockChar++;
        cursorPos++;

        if (blockChar == SimpleDisk::BLOCK_SIZE) {  // going to the next block
            Console::puts("Going to the next INode\n");
            if (fileINode->nextINode == 0) {
                Console::puts("MAYDAY at Read File. There is no next INode\n");
                assert(false);
            }
            fileINode = currentFileSystem->inodesInode(fileINode->nextINode);      // moving the fileINode to the next part of the file
            currentDisk->read(fileINode->blockID, (unsigned char *)blockContent);  // ILLELGAL TYPE-CAST
            blockChar = 0;
        }
    }
    return counter;
}

int File::Write(unsigned int _n, const char *_buf) {
    Console::puts("writing to file\n");

    if (_n + cursorPos > 64 * 1024) {
        Console::puts("MAYDAY at File Read. Out of range\n");
        assert(false);
    }

    char *blockContent = new char[SimpleDisk::BLOCK_SIZE];
    if (cursorPos > 0)
        currentDisk->read(fileINode->blockID, (unsigned char *)blockContent);  // ILLELGAL TYPE-CAST
    for (int i = cursorPos; i < SimpleDisk::BLOCK_SIZE; i++) {                 // reading the content prior to the cursor
        blockContent[i] = NULL;
    }
    int counter;
    int blockChar = cursorPos % SimpleDisk::BLOCK_SIZE;
    Console::puts("\n CH1-");
    Console::putui(fileINode->fileSizeChar);
    for (counter = 0; counter < _n; counter++) {
        blockContent[blockChar] = _buf[counter];
        blockChar++;
        cursorPos++;
        fileINode->fileSizeChar = fileINode->fileSizeChar + 1;

        if (blockChar == SimpleDisk::BLOCK_SIZE) {  // going to the next block
            Console::puts("Going to the next INode\n");
            if (fileINode->nextINode == 0) {
                Console::puts("MAYDAY at Read File. There is no next INode\n");
                assert(false);
            }
            if (!currentFileSystem->Save()) {
                Console::puts("MAYDAY at write file. Not able to save the INode data\n");
                assert(false);
            }
            currentDisk->write(fileINode->blockID, (unsigned char *)blockContent);  // ILLELGAL TYPE-CAST
            fileINode = currentFileSystem->inodesInode(fileINode->nextINode);       // moving the fileINode to the next part of the file
            blockChar = 0;
        }
    }
    if (!currentFileSystem->Save()) {
        Console::puts("MAYDAY at write file. Not able to save the INode data\n");
        assert(false);
    }
    currentDisk->write(fileINode->blockID, (unsigned char *)blockContent);  // ILLELGAL TYPE-CAST
    return counter;
}

void File::Reset() {
    Console::puts("resetting file\n");

    fileINode = currentFileSystem->LookupFile(currentFileID);
    cursorPos = 0;
}

bool File::EoF() {
    Console::puts("checking for EoF\n");
    
    int totalFileSize = 0;
    Inode *tempInode = currentFileSystem->LookupFile(currentFileID);
    while (true) {
        totalFileSize += tempInode->fileSizeChar;
        if (tempInode->nextINode == 0)//end of the chain
            break;
        tempInode = currentFileSystem->inodesInode(tempInode->nextINode);
    }

    if (cursorPos == totalFileSize)
        return true;
    return false;
}