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
    Inode *fileINode = currentFileSystem->LookupFile(_id);
    currentDisk = currentFileSystem->diskgetter();
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
    currentDisk->read(fileINode->blockID, fileContent);
    for (int i = 0; i < _n; i++) {
        if (i + cursorPos > fileSize) {
            Console::puts("MAYDAY at File Read, requested more char than the file length");
            return readChar;
        }
        _buf[i] = fileContent[i + cursorPos];
        readChar++;
    }
    cursorPos+=readChar;
    return readChar;
}

int File::Write(unsigned int _n, const char *_buf) {
    Console::puts("writing to file\n");
    char* fileContent=new char[SimpleDisk::BLOCK_SIZE];
    if(cursorPos!=0)
        Read(cursorPos,fileContent);//reading the content of the file prior to the cursorposition

    unsigned short writeChar = 0;
    
    for (int i = 0; i < _n; i++) {
        if (i + cursorPos > fileSize) {
            Console::puts("MAYDAY at File Read, requested more char than the file length");
            return writeChar;
        }
        fileContent[i+cursorPos] = _buf[i];
        writeChar++;
    }
    currentDisk->write(fileINode->blockID, fileContent);
    cursorPos+=writeChar;
    return writeChar;
}

void File::Reset() {
    Console::puts("resetting file\n");
    cursorPos = 0;
}

bool File::EoF() {
    Console::puts("checking for EoF\n");
    if (cursorPos == fileSize)
        return true;
    return false;
}
