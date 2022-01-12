#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include "disk.h"

#ifndef __FS_H__
#define __FS_H__

#define ROOT_BLOCK 0
#define FAT_BLOCK 1
#define FAT_FREE 0
#define FAT_EOF -1

#define TYPE_FILE 0
#define TYPE_DIR 1
#define READ 0x04
#define WRITE 0x02
#define EXECUTE 0x01


// TODO
// prevent CP copying file into a dir where a file of that name already exists.
// same as above for MV
// make it possible to MV/CP files to the directory above ".."
// FIX MEMORY POINTER UNINTIALIZED SUPER AIDS
//
//

// Constant for comparing
static const std::string DOTDOT = "..";

struct dir_entry {
    char file_name[56] = ""; // name of the file / sub-directory
    uint32_t size = 0; // size of the file in bytes
    uint16_t first_blk = 0; // index in the FAT for the first block of the file
    uint8_t type = 0; // directory (1) or file (0)
    uint8_t access_rights = 0; // read (0x04), write (0x02), execute (0x01)
};

struct treeNode
{
    treeNode* parent;
    dir_entry* entry;
    std::vector<treeNode*> children;
    treeNode()
    {
        parent = nullptr;
        entry = nullptr;
    }
    treeNode(treeNode *parent)
    {
        this->parent = parent;
        entry = nullptr;
    }
    treeNode(treeNode *parent, dir_entry *entry)
    {
        this->parent = parent;
        // copy over the dir entry
        dir_entry *newEntry = new dir_entry;
        for (int i = 0; i < 56; i++)
        {
            newEntry->file_name[i] = entry->file_name[i];
        }
        newEntry->first_blk = entry->first_blk;
        newEntry->size = entry->size;
        newEntry->access_rights = entry->access_rights;
        newEntry->type = entry->type;

        this->entry = newEntry;
    }
};

class FS {
private:
    Disk disk;
    // size of a FAT entry is 2 bytes
    int16_t fat[BLOCK_SIZE/2];
    // size of a dir_entry is 64 bytes
    std::vector<dir_entry*> workingDir;
    treeNode *root = nullptr;
    treeNode *currentNode = nullptr;
    void cleanUp();
    void cleanUpDirs(treeNode* branch);
    void cleanUpFiles();
    void deleteWorkingDir();
    void updateFat();
    void readInFatRoot();
    void initWorkingDir(uint16_t blk);
    void changeWorkingDir(uint16_t blk);
    void initTree();
    void initTreeContinued(treeNode *branch);
    void writeWorkingDirToBlock(uint16_t blk);
    dir_entry* copyDirEntry(dir_entry* dir);
    dir_entry* copyDirEntry(dir_entry* dir, std::string name);
    dir_entry* copyDirEntry(dir_entry* dir, std::string name, uint16_t first_blk);
    dir_entry* makeDotDotDir(uint16_t blk);

    // parses a filepath and calls changeDirectory()
    // to change current workingDir to last directory in path
    // if it exists. if doesnt exist it returns -1
    int parsePath(std::string path);
    std::string parseTilFile(std::string path);
    // checks if file exists and is a directory, then changes directory
    // return -1 if it doesnt exists or is a file.
    int changeDirectory(std::string dirName);
    // returns a std:string vector containg all the dirs/files in a given path.
    std::vector<std::string> splitPath(std::string path);

    uint16_t convert8to16(uint8_t num1, uint8_t num2);
    void convert16to8(uint16_t num, uint8_t * result);
    int getSecondNum(uint16_t num);
    uint32_t convert8to32(uint8_t *result);
    void convert32to8(uint32_t num, uint8_t *result);
    // help function for cp return first block index
    int writeBlocksFromString(std::string contents);
    //Writes to already existing block from string
    int writeBlocksFromString
        (std::string filepath, std::string contents, uint16_t startFatIndex, int blockIndex);
    // return index of first block, -1 if not found
    int findBlockWorkingDir(std::string filename);
    // returns index in workingDir array, -1 if not found
    int findIndexWorkingDir(std::string filename);
    // check if file exists
    bool fileExist(std::string filename);
    // Finds end of file both block index and end in said block
    void findEOF(uint16_t first_blk, uint16_t *result);
    //Checks if dir is empty
    bool dirEmpty(uint16_t blk);

public:
    FS();
    ~FS();
    // formats the disk, i.e., creates an empty file system
    int format();
    // create <filepath> creates a new file on the disk, the data content is
    // written on the following rows (ended with an empty row)
    int create(std::string filepath);
    // cat <filepath> reads the content of a file and prints it on the screen
    int cat(std::string filepath);
    // ls lists the content in the currect directory (files and sub-directories)
    int ls();

    int getFreeIndex();

    void testDisk();

    // cp <sourcepath> <destpath> makes an exact copy of the file
    // <sourcepath> to a new file <destpath>
    int cp(std::string sourcepath, std::string destpath);
    // mv <sourcepath> <destpath> renames the file <sourcepath> to the name <destpath>,
    // or moves the file <sourcepath> to the directory <destpath> (if dest is a directory)
    int mv(std::string sourcepath, std::string destpath);
    // rm <filepath> removes / deletes the file <filepath>
    int rm(std::string filepath);
    // append <filepath1> <filepath2> appends the contents of file <filepath1> to
    // the end of file <filepath2>. The file <filepath1> is unchanged.
    int append(std::string filepath1, std::string filepath2);

    // mkdir <dirpath> creates a new sub-directory with the name <dirpath>
    // in the current directory
    int mkdir(std::string dirpath);
    // cd <dirpath> changes the current (working) directory to the directory named <dirpath>
    int cd(std::string dirpath);
    // pwd prints the full path, i.e., from the root directory, to the current
    // directory, including the currect directory name
    int pwd();

    // chmod <accessrights> <filepath> changes the access rights for the
    // file <filepath> to <accessrights>.
    int chmod(std::string accessrights, std::string filepath);
};

#endif // __FS_H__
