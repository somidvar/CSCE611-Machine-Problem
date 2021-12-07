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
    int numBlockToBeRead, charInBlockToBeRead;
    int readChar = 0;
    char *blockContent = new char[SimpleDisk::BLOCK_SIZE];
    for (int i = 0; i < _n; i++)
        _buf[i] = NULL;  // cleaning the buffer

    int firstBlockToBeRead, lastBlockToBeRead;
    firstBlockToBeRead = cursorPos / SimpleDisk::BLOCK_SIZE;
    lastBlockToBeRead = (_n + cursorPos) / SimpleDisk::BLOCK_SIZE;
    numBlockToBeRead = lastBlockToBeRead - firstBlockToBeRead + 1;

    for (int j = 0; j < numBlockToBeRead; j++) {
        if (_n + cursorPos > SimpleDisk::BLOCK_SIZE)
            charInBlockToBeRead = SimpleDisk::BLOCK_SIZE - cursorPos;
        else
            charInBlockToBeRead = _n;
        currentDisk->read(fileINode->blockID, (unsigned char *)blockContent);  // ILLEGAL TYPE-CAST

        for (int i = 0; i < charInBlockToBeRead; i++) {
            _buf[readChar] = blockContent[i + cursorPos];
            readChar++;
        }
        cursorPos += charInBlockToBeRead;
        if (cursorPos == SimpleDisk::BLOCK_SIZE) {                             // moving to the next block
            fileINode = currentFileSystem->inodesInode(fileINode->nextINode);  // moving the fileINode to the next part of the file
            cursorPos = 0;
            _n -= charInBlockToBeRead;
        }
    }
    return readChar;
}

int File::Write(unsigned int _n, const char *_buf) {
    Console::puts("writing to file\n");

    char *fileContent = new char[SimpleDisk::BLOCK_SIZE];
    for (int i = 0; i < cursorPos; i++) {
        fileContent[i] = block_cache[i];  // reading the content of the file prior to the cursor pointer
    }

    unsigned short writeChar = 0;
    for (int i = 0; i < _n; i++) {
        if (i + cursorPos > SimpleDisk::BLOCK_SIZE - 1) {  // the last char is reserved for EOF
            Console::puts("MAYDAY at File Write, requested more char than the file length");
            break;
        }
        fileContent[i + cursorPos] = _buf[i];
        writeChar++;
    }
    cursorPos += writeChar;
    fileContent[cursorPos] = 5;                                            // setting the EOF character
    currentDisk->write(fileINode->blockID, (unsigned char *)fileContent);  // ILLEGAL TYPE-CAST
    for (int i = 0; i < SimpleDisk::BLOCK_SIZE; i++)
        block_cache[i] = fileContent[i];  // updating the cache of the file;
    return writeChar;
}

void File::Reset() {
    Console::puts("resetting file\n");

    fileINode = currentFileSystem->LookupFile(currentFileID);
    cursorPos = 0;
}

bool File::EoF() {
    Console::puts("checking for EoF\n");
    
    if (cursorPos == fileINode->fileSize)
        return true;
    return false;
}