#include <cstdint>
#include <iostream>
#include <string>
#include "fs.h"

FS::FS()
{
    std::cout << "FS::FS()... Creating file system\n";
    readInFatRoot();
}

FS::~FS()
{
    updateFatRoot();
}

int getSecondNum(uint16_t num)
{
    int i;
    for (i = 0; ((i + 1) * 255) < num; i++)
        ;
    return i;
}

uint32_t convert8to32(uint8_t *result)
{
    return (result[0] << 24) | (result[1] << 16) | (result[2] << 8) | result[3];
}

void convert32to8(uint32_t num, uint8_t *result)
{
    result[0] = (num & 0xff000000UL) >> 24;
    result[1] = (num & 0x00ff0000UL) >> 16;
    result[2] = (num & 0x0000ff00UL) >> 8;
    result[3] = (num & 0x000000ffUL);
}

void FS::convert16to8(uint16_t num, uint8_t *result)
{
    result[0] = *((uint8_t*)&(num)+1);
    result[1] = *((uint8_t*)&(num)+0);
}

uint16_t FS::convert8to16(uint8_t num1, uint8_t num2)
{
    uint16_t returnVal = (num1 << 8) | num2;
    return returnVal;
}

void FS::updateFatRoot()
{
    uint8_t block[4096];
    uint8_t bit16[2];
    uint8_t bit32[4];
    disk.write(0, block);
    disk.write(1, block);
    int x = 0;
    // take each FAT array entry and split it into two 8bit (1 byte)
    // entries which are placed in the block at index x and x+1
    for (int i = 0; i < BLOCK_SIZE / 2;i++)
    {
        convert16to8(fat[i], bit16);
        block[x] = bit16[0];
        block[x + 1] = bit16[1];
        x += 2;
    }
    // write the FAT block
    disk.write(1, block);

    // reset the block array.
    for (int i = 0; i < 4096; i++)
    {
        block[i] = 0;
    }

    // reset x-counter
    x = 0;

    // size of a dir_entry is 64 bytes
    for (int i = 0; i < entries.size(); i++)
    {
        // loop through file_name char array
        // adding each char into the block array
        for (int j = 0; j < 56; j++)
        {
            block[x] = entries[i]->file_name[j];
            x++;
        }
        // convert one 32 bit (4 bytes) INT to four 8bit (1 byte) INTs
        // saved in var "bit32"
        convert32to8(entries[i]->size, bit32);
        // add each of the four 8bit (1 byte) INTs to the block.
        for (int j = 0; j < 4; j++)
        {
            block[x] = bit32[j];
            x++;
        }
        // convert 16bit (2 byte) first_blk into two
        // 8bit (1 byte) INTs
        convert16to8(entries[i]->first_blk, bit16);
        // add each of the two 8bit (1 byte) INTs to the block.
        for (int j = 0; j < 2; j++)
        {
            block[x] = bit16[j];
            x++;
        }
        //add the type which is already a 8bit (1 byte) INT to the block.
        block[x] = entries[i]->type;
        x++;
        //add the access_rights which is already a 8bit (1 byte) INT to the block.
        block[x] = entries[i]->access_rights;
        x++;
    }
    // write the dir_entry block
    disk.write(0, block);
}

void FS::readInFatRoot()
{
    uint8_t block[4096];
    // read the FAT block into block array
    disk.read(1, block);
    // counter
    int x = 0;

    // loop through half the size of the FAT block
    // take 2 bytes each iteration and converting them
    // too a 16bit (2 byte) INT and adding it to the FAT array
    // until we have read the whole FAT from file
    for (int i = 0; i < BLOCK_SIZE / 2;i++)
    {
        fat[i] = convert8to16(block[x], block[x + 1]);
        x += 2;
    }
    // reset the block array
    for (int i = 0;i < 4096;i++)
    {
        block[i] = 0;
    }
    // read the dir_entry block into block array
    disk.read(0, block);
    dir_entry *newDir;
    uint8_t result[4];

    // loop through dir_entry block until we reach end '\0'
    // jumps 64 at each iteration since size of dir_entry is 64 bytes.
    for (int i = 0;i < 4096 && block[i] != '\0';i += 64)
    {
        // reset x to point to the first byte of the next dir entry
        int x = i;
        // create a new dir
        newDir = new dir_entry;

        // loop through the 56 bytes of the filename
        // copy it to newDir filename
        for (int j = 0;j < 56;j++)
        {
            newDir->file_name[j] = block[x];
            x++;
        }
        // copy the next 4 bytes containing size to reslut array
        for (int j = 0;j < 4;j++)
        {
            result[j] = block[x];
            x++;
        }
        // convert the 4 bytes in result array into a 32bit (4 byte) INT
        // and copy into newDirs size
        newDir->size = convert8to32(result);
        // copy the next 2 bytes containing first_blk into result array
        for (int j = 0;j < 2;j++)
        {
            result[j] = block[x];
            x++;
        }
        // convert the 2 bytes into one 16bit (2 byte) INT
        // and copy it to first_blk
        newDir->first_blk = convert8to16(result[0], result[1]);
        // copy the next 1 byte straight into the type variable
        // as it is already a 8bit (1 byte) INT, no conversion needed.
        newDir->type = block[x];
        x++;
        // do the same as for type above for the access_rights.
        newDir->access_rights = block[x];
        // push the entry into the entries array.
        entries.push_back(newDir);
    }
}

// returns index in entries array, -1 if not found
int FS::findFileinEntries(std::string filename)
{
    bool found = false;
    //Tries to find file in entries
    int index = -1;
    uint16_t first_blk = 0;
    for (int i = 0;i < entries.size();i++)
    {
        if (entries[i]->file_name == filename)
        {
            found = true;
            index = i;
            break;
        }
    }
    // return index
    return index;
}
// return index of first block
int FS::findFileInRoot(std::string filename)
{
    bool found = false;
    //Tries to find file in rootblock
    uint16_t first_blk = 0;
    for (int i = 0;i < entries.size();i++)
    {
        if (entries[i]->file_name == filename)
        {
            first_blk = entries[i]->first_blk;
            found = true;
            break;
        }
    }
    // return index of first block
    return first_blk;
}

bool FS::fileExist(std::string filename)
{
    bool found = false;
    //Tries to find file
    for (int i = 0;i < entries.size();i++)
    {
        if (entries[i]->file_name == filename)
        {
            found = true;
            break;
        }
    }
    // return index of first block
    return found;
}

// formats the disk, i.e., creates an empty file system
int FS::format()
{
    uint8_t block[4096];
    disk.write(0, block);
    disk.write(1, block);
    fat[ROOT_BLOCK] = FAT_EOF;
    fat[FAT_BLOCK] = FAT_EOF;
    for (int i = 2; i < BLOCK_SIZE / 2; i++)
    {
        fat[i] = FAT_FREE;
    }
    entries.clear();
    dir_entry *rootDir = new dir_entry;
    rootDir->file_name[0] = '/';
    rootDir->first_blk = 0;
    rootDir->size = 64;
    rootDir->type = 1;
    rootDir->access_rights = 0x06;
    entries.push_back(rootDir);

    updateFatRoot();

    return 0;
}

// return first free block index
int FS::getFreeIndex()
{
    for (int i = 0; i < BLOCK_SIZE / 2; i++)
    {
        if (fat[i] == FAT_FREE)
        {
            return i;
        }
    }

    return -2;
}

void FS::testDisk()
{
    for (int i = 0; i < entries.size(); i++)
    {
        std::cout << (entries[i]->file_name + '\0') << std::endl;
    }
    uint8_t block[4096];
    disk.read(1, block);
    for (int i = 0;i < 4096 && block[i] != '\0';i++)
    {
    }
    for (int i = 0;i < 10;i++)
    {
        std::cout << fat[i] << ',';
    }
}

// help function for cp return first block index
int FS::writeBlocksFromString(std::string filepath, std::string contents)
{
    uint8_t block[4096];
    int firstFatIndex = 0;
    int prevIndex = FAT_EOF;

    // start writing blocks
    int count = 0;
    int fatIndex = 0;
    fatIndex = getFreeIndex();
    firstFatIndex = fatIndex;
    for (int i = 0; i < contents.size(); i++)
    {
        // if file is bigger than size of 1 block (4096 bytes)
        if (count >= 4096)
        {
            // write block to file
            disk.write(fatIndex, block);
            // save previous fatIndex
            prevIndex = fatIndex;
            // set prevIndex as EOF temporarily so
            // so getFreeIndex doesnt choose it.
            fat[prevIndex] = FAT_EOF;
            // get a new free block index
            fatIndex = getFreeIndex();
            // set prev FAT index next block as current fatIndex
            fat[prevIndex] = fatIndex;
            // reset block
            for (int j = 0; j < 4096; j++)
            {
                block[j] = 0;
            }
            // reset count so we can continue
            // writing the remaining file contents to the reset block
            count = 0;
            //std::cout << "fatIndex: " << fatIndex << std::endl;
            //std::cout << "prevIndex: " << prevIndex << std::endl;
        }
        // add each char from file contents into block.
        block[count] = contents[i];
        count++;
    }

    // write last block
    fat[fatIndex] = FAT_EOF;
    disk.write(fatIndex, block);

    std::cout << "Added contents to blocks\n";
    return firstFatIndex;
}

// create <filepath> creates a new file on the disk, the data content is
// written on the following rows (ended with an empty row)
int FS::create(std::string filepath)
{

    // throw error if file already exists
    if(fileExist(filepath)){
        return 1;
    }

    std::cin.clear();
    std::cout << "FS::create(" << filepath << ")\n";
    std::string contents;
    std::string row = "";
    uint8_t block[4096];
    int firstFatIndex = 0;
    int prevIndex = FAT_EOF;

    // take user input for file contents one row at a time
    // until user enters an empty row and we break the loop.
    while (true)
    {
        std::getline(std::cin, row);
        if (row == "")
        {
            break;
        }
        row.push_back('\n');
        contents.append(row);
    }
    // add null termination to end of file.
    contents.push_back('\0');

    // create new file and save its first block.
    firstFatIndex = writeBlocksFromString(filepath, contents);

    std::cout << "Added contents to blocks\n";

    dir_entry *newEntry = new dir_entry;
    for (int i = 0; i < 56 && i < filepath.size() + 1; i++)
    {
        newEntry->file_name[i] = filepath[i];
    }
    newEntry->first_blk = firstFatIndex;
    newEntry->size = contents.size();
    newEntry->access_rights = 0x06;
    newEntry->type = 0;
    entries.push_back(newEntry);
    std::cout << "Added contents to dir entries\n";

    updateFatRoot();

    return 0;
}

// cat <filepath> reads the content of a file and prints it on the screen
int FS::cat(std::string filepath)
{
    std::cout << "FS::cat(" << filepath << ")\n";
    //Tries to find file in rootblock
    uint16_t first_blk = findFileInRoot(filepath);

    // if file cannot be found, throw error.
    if(first_blk == 0){
        return 1;
    }

    uint8_t block[4096];
    int fatIndex = first_blk;
    while (fatIndex != FAT_EOF && first_blk != 0)
    {
        std::cout << "Current Block Index: " << fatIndex << std::endl;
        disk.read(fatIndex, block);
        for (int i = 0;i < 4096 && block[i] != '\0';i++)
        {
            std::cout << block[i];
        }
        fatIndex = fat[fatIndex];
    }
    return 0;
}

// ls lists the content in the currect directory (files and sub-directories)
int FS::ls()
{
    std::cout << "FS::ls()\n";
    std::cout << "name\tsize\n";
    for (int i = 0;i < entries.size();i++)
    {
        std::cout << entries[i]->file_name << '\t' << entries[i]->size << '\n';
    }
    return 0;
}

// cp <sourcepath> <destpath> makes an exact copy of the file
// <sourcepath> to a new file <destpath>
int FS::cp(std::string sourcepath, std::string destpath)
{
    std::cout << "FS::cp(" << sourcepath << "," << destpath << ")\n";
    //Tries to find file in rootblock

    uint16_t first_blk = 0;
    uint8_t block[4096];
    int dirEntryIndex = 0;
    std::string contents = "";
    //Tries to find file in rootblock
    first_blk = findFileInRoot(sourcepath);

    // if file cannot be found, throw error.
    if(first_blk == 0){
        return 1;
    }
    // throw error if destination file already exists
    if(fileExist(destpath)){
        return 1;
    }

    // read in all the from the sourcefile blocks to contents.
    int fatIndex = first_blk;
    while (fatIndex != FAT_EOF && first_blk != 0)
    {
        //std::cout << "fatIndex: " << fatIndex << std::endl;
        disk.read(fatIndex, block);
        for (int i = 0;i < 4096 && block[i] != '\0';i++)
        {
            contents.push_back(block[i]);
        }
        fatIndex = fat[fatIndex];
    }
    // create new file and save its first block.
    first_blk = writeBlocksFromString(destpath, contents);

    // find sourcefile dir_entry
    dirEntryIndex = findFileinEntries(sourcepath);

    // copy over the dir entry
    dir_entry *newEntry = new dir_entry;
    for (int i = 0; i < 56 && i < destpath.size() + 1; i++)
    {
        newEntry->file_name[i] = destpath[i];
    }
    newEntry->first_blk = first_blk;
    newEntry->size = entries[dirEntryIndex]->size;
    newEntry->access_rights = entries[dirEntryIndex]->access_rights;
    newEntry->type = entries[dirEntryIndex]->type;
    entries.push_back(newEntry);

    // save to disk
    updateFatRoot();

    return 0;
}

// mv <sourcepath> <destpath> renames the file <sourcepath> to the name <destpath>,
// or moves the file <sourcepath> to the directory <destpath> (if dest is a directory)
int FS::mv(std::string sourcepath, std::string destpath)
{
    std::cout << "FS::mv(" << sourcepath << "," << destpath << ")\n";
    return 0;
}

// rm <filepath> removes / deletes the file <filepath>
int FS::rm(std::string filepath)
{
    std::cout << "FS::rm(" << filepath << ")\n";
    return 0;
}

// append <filepath1> <filepath2> appends the contents of file <filepath1> to
// the end of file <filepath2>. The file <filepath1> is unchanged.
int FS::append(std::string filepath1, std::string filepath2)
{
    std::cout << "FS::append(" << filepath1 << "," << filepath2 << ")\n";
    return 0;
}

// mkdir <dirpath> creates a new sub-directory with the name <dirpath>
// in the current directory
int FS::mkdir(std::string dirpath)
{
    std::cout << "FS::mkdir(" << dirpath << ")\n";
    return 0;
}

// cd <dirpath> changes the current (working) directory to the directory named <dirpath>
int FS::cd(std::string dirpath)
{
    std::cout << "FS::cd(" << dirpath << ")\n";
    return 0;
}

// pwd prints the full path, i.e., from the root directory, to the current
// directory, including the currect directory name
int FS::pwd()
{
    std::cout << "FS::pwd()\n";
    return 0;
}

// chmod <accessrights> <filepath> changes the access rights for the
// file <filepath> to <accessrights>.
int FS::chmod(std::string accessrights, std::string filepath)
{
    std::cout << "FS::chmod(" << accessrights << "," << filepath << ")\n";
    return 0;
}
