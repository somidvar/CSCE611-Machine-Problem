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
    fileINode = currentFileSystem->LookupFile(_id);
    if (fileINode == NULL) {
        Console::puts("The file should have been created but it cannot be found\n");
        assert(false);
    }
    currentDisk = currentFileSystem->diskgetter();
    cursorPos = 0;
    currentDisk->read(fileINode->blockID, block_cache);
    fileSize = FileSizeGetter();
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
    for (int i = 0; i < _n; i++)
        _buf[i] = NULL;

    unsigned short readChar = 0;
    char *fileContent = new char[SimpleDisk::BLOCK_SIZE];
    currentDisk->read(fileINode->blockID, (unsigned char *)fileContent);  // ILLEGAL TYPE-CAST

    Console::putch(fileContent[0]);
    Console::putch(fileContent[1]);
    Console::putch(fileContent[2]);
    Console::putch(fileContent[3]);
    for (int i = 0; i < _n; i++) {
        if (i + cursorPos > SimpleDisk::BLOCK_SIZE - 1) {  // the last char is reserved for EOF
            Console::puts("MAYDAY at File Read, requested more char than the file length");
            return readChar;
        }
        _buf[i] = fileContent[i + cursorPos];
        readChar++;
    }
    cursorPos += readChar;
    return readChar;
}

int File::Write(unsigned int _n, const char *_buf) {
    Console::puts("writing to file\n");
    char *fileContent = new char[SimpleDisk::BLOCK_SIZE];

    for (int i = 0; i < cursorPos; i++) {
        fileContent[i] = block_cache[i];  // reading the content of the file prior to the cursor pointer
        Console::puts("CH1\n");
    }

    unsigned short writeChar = 0;
    Console::puts("CH2\n");
    Console::putch(_buf[0]);
    Console::putch(_buf[1]);
    Console::putch(_buf[2]);
    Console::putch(_buf[3]);
    Console::puts("CH3\n");
    Console::puti(fileSize);
    Console::puts("\n");
    Console::puti(_n);
    Console::puts("\n");
    Console::puti(cursorPos);
    Console::puts("CH4\n");
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
    cursorPos = 0;
}

bool File::EoF() {
    Console::puts("checking for EoF\n");
    if (cursorPos == FileSizeGetter())
        return true;
    return false;
}
int File::FileSizeGetter() {
    int size = 0;
    unsigned char *fileContent = new unsigned char[SimpleDisk::BLOCK_SIZE];
    currentDisk->read(fileINode->blockID, fileContent);
    for (int i = 0; i < SimpleDisk::BLOCK_SIZE; i++) {
        if (fileContent[i] == 5) {  // character that I chose for the end of file
            return i;
        }
    }
    return 0;
}