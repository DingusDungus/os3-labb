#include <iostream>
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

uint32_t convert8to32(uint8_t *result)
{
    //Converts a set of 8 bit integers into one whole 32 bit integer
    return (result[0] << 24) | (result[1] << 16) | (result[2] << 8) | result[3];
}

void convert32to8(uint32_t num, uint8_t *result)
{
    //Converts a 30 bit integer to 4 8 bit integers
    result[0] = (num & 0xff000000UL) >> 24;
    result[1] = (num & 0x00ff0000UL) >> 16;
    result[2] = (num & 0x0000ff00UL) >> 8;
    result[3] = (num & 0x000000ffUL);
}

void FS::convert16to8(uint16_t num, uint8_t *result)
{
    //Converts a 16 bit integer into a set of 8 bit integers
    result[0] = *((uint8_t *)&(num) + 1);
    result[1] = *((uint8_t *)&(num) + 0);
}

uint16_t FS::convert8to16(uint8_t num1, uint8_t num2)
{
    //Converts a set of 8 bit integers into one 16 bit integer
    uint16_t returnVal = (num1 << 8) | num2;
    return returnVal;
}

void FS::updateFatRoot()
{
    //updates both the FAT and root block on the disk so they stay up to date
    uint8_t block[4096];
    uint8_t bit16[2];
    uint8_t bit32[4];
    disk.write(0, block);
    disk.write(1, block);
    int x = 0;
    // take each FAT array entry and split it into two 8bit (1 byte)
    // entries which are placed in the block at index x and x+1
    for (int i = 0; i < BLOCK_SIZE / 2; i++)
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
    for (int i = 0; i < BLOCK_SIZE / 2; i++)
    {
        fat[i] = convert8to16(block[x], block[x + 1]);
        x += 2;
    }
    // reset the block array
    for (int i = 0; i < 4096; i++)
    {
        block[i] = 0;
    }
    // read the dir_entry block into block array
    disk.read(0, block);
    dir_entry *newDir;
    uint8_t result[4];

    // loop through dir_entry block until we reach end '\0'
    // jumps 64 at each iteration since size of dir_entry is 64 bytes.
    for (int i = 0; i < 4096 && block[i] != '\0'; i += 64)
    {
        int x = i;
        newDir = new dir_entry;
        for (int j = 0; j < 56; j++)
        {
            newDir->file_name[j] = block[x];
            x++;
        }
        for (int j = 0; j < 4; j++)
        {
            result[j] = block[x];
            x++;
        }
        newDir->size = convert8to32(result);
        for (int j = 0; j < 2; j++)
        {
            result[j] = block[x];
            x++;
        }
        newDir->first_blk = convert8to16(result[0], result[1]);
        newDir->type = block[x];
        x++;
        newDir->access_rights = block[x];
        entries.push_back(newDir);
    }
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

int FS::getFreeIndex()
{
    //Returns first index found which is free on the FAT
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
    //Testing func for the devs
    for (int i = 0; i < entries.size(); i++)
    {
        std::cout << (entries[i]->file_name + '\0') << std::endl;
    }
    uint8_t block[4096];
    disk.read(1, block);
    for (int i = 0; i < 4096 && block[i] != '\0'; i++)
    {
    }
    for (int i = 0; i < 10; i++)
    {
        std::cout << fat[i] << ',';
    }
}

// create <filepath> creates a new file on the disk, the data content is
// written on the following rows (ended with an empty row)
int FS::create(std::string filepath)
{
    std::cin.clear();
    std::cout << "FS::create(" << filepath << ")\n";
    std::string contents;
    std::string row = " ";
    uint8_t block[4096];
    int firstFatIndex = 0;
    int pred = FAT_EOF;
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
    int count = 0;
    int fatIndex = 0;
    for (int i = 0; i < contents.size(); i++)
    {
        if (count >= 4096)
        {
            fatIndex = getFreeIndex();
            if (pred != FAT_EOF)
            {
                fat[pred] = fatIndex;
            }
            else
            {
                firstFatIndex = fatIndex;
                fat[fatIndex] = pred;
            }
            fat[fatIndex] = FAT_EOF;
            disk.write(fatIndex, block);
            for (int j = 0; j < 4096; j++)
            {
                block[0] = 0;
            }
            count = 0;
        }
        block[count] = contents[i];
        count++;
    }

    fatIndex = getFreeIndex();
    if (pred != FAT_EOF)
    {
        fat[pred] = fatIndex;
    }
    else
    {
        firstFatIndex = fatIndex;
        fat[fatIndex] = pred;
    }
    disk.write(fatIndex, block);

    std::cout << "Added contents to blocks\n";

    dir_entry *newEntry = new dir_entry;
    for (int i = 0; i < 56 && i < filepath.size(); i++)
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

    bool isEqual = true;
    uint16_t first_blk = 0;
    uint8_t block[4096];
    int dirEntryIndex = 0;
    for (int i = 0; i < entries.size(); i++)
    {
        for (int j = 0; entries[i]->file_name[j] != '\0' && j < 56; j++)
        {
            if (entries[i]->file_name[j] != filepath[j])
            {
                isEqual = false;
                break;
            }
        }
        if (isEqual)
        {
            first_blk = entries[i]->first_blk;
            dirEntryIndex = i;
            break;
        }
        else
        {
            isEqual = true;
        }
    }
    int fatIndex = first_blk;
    while (fatIndex != FAT_EOF && first_blk != 0)
    {
        disk.read(fatIndex, block);
        for (int i = 0; i < 4096 && i < entries[dirEntryIndex]->size; i++)
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
    for (int i = 0; i < entries.size(); i++)
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

    bool isEqual = true;
    uint16_t first_blk = 0;
    uint8_t block[4096];
    int dirEntryIndex = 0;
    for (int i = 0; i < entries.size(); i++)
    {
        for (int j = 0; entries[i]->file_name[j] != '\0' && j < 56; j++)
        {
            if (entries[i]->file_name[j] != sourcepath[j])
            {
                isEqual = false;
                break;
            }
        }
        if (isEqual)
        {
            first_blk = entries[i]->first_blk;
            dirEntryIndex = i;
            break;
        }
        else
        {
            isEqual = true;
        }
    }
    int fatIndex = first_blk;
    int cpIndex = 0;
    int pred = FAT_EOF;
    int firstFatIndex = 0;
    while (fatIndex != FAT_EOF && first_blk != 0)
    {
        disk.read(fatIndex, block);
        cpIndex = getFreeIndex();
        if (pred != FAT_EOF)
        {
            fat[pred] = cpIndex;
        }
        else
        {
            firstFatIndex = cpIndex;
            fat[cpIndex] = pred;
        }
        disk.write(cpIndex, block);
        fatIndex = fat[fatIndex];
    }
    fat[cpIndex] = FAT_EOF;
    dir_entry *newEntry = new dir_entry;
    for (int i = 0; i < 56 && i < destpath.size(); i++)
    {
        newEntry->file_name[i] = destpath[i];
    }
    newEntry->first_blk = firstFatIndex;
    newEntry->size = entries[dirEntryIndex]->size;
    newEntry->access_rights = entries[dirEntryIndex]->access_rights;
    newEntry->type = entries[dirEntryIndex]->type;
    entries.push_back(newEntry);

    updateFatRoot();

    return 0;
}

// mv <sourcepath> <destpath> renames the file <sourcepath> to the name <destpath>,
// or moves the file <sourcepath> to the directory <destpath> (if dest is a directory)
int FS::mv(std::string sourcepath, std::string destpath)
{
    std::cout << "FS::mv(" << sourcepath << "," << destpath << ")\n";
    bool isEqual = true;
    int dirEntryIndex = 0;
    for (int i = 0; i < entries.size(); i++)
    {
        for (int j = 0; entries[i]->file_name[j] != '\0' && j < 56; j++)
        {
            if (entries[i]->file_name[j] != sourcepath[j])
            {
                isEqual = false;
                break;
            }
        }
        if (isEqual)
        {
            dirEntryIndex = i;
            break;
        }
        else
        {
            isEqual = true;
        }
        for (int i = 0;i < 56;i++)
        {
            entries[dirEntryIndex]->file_name[i] = 0;
        }
        for (int i = 0;i < destpath.size();i++)
        {
            entries[dirEntryIndex]->file_name[i] = destpath[i];
        }
    }
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
