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

int getSecondNum(uint16_t num)
{
    int i;
    for (i = 0;((i + 1) * 255) < num;i++);
    return i;
}

void FS::fatDiskIndexCalc(uint16_t num, uint8_t *result)
{

    result[0] = getSecondNum(num);
    result[1] = num % 255;
}

uint16_t FS::fatDiskIndexCalc(uint8_t num1, uint8_t num2)
{
    return (255 * num1 + num2);
}

void FS::updateFatRoot()
{
    uint8_t block[4096];
    uint8_t indexes[2];
    disk.write(0, block);
    disk.write(1, block);
    for (int i = 0;i < BLOCK_SIZE/2;i += 2)
    {
        fatDiskIndexCalc(fat[i], indexes);
        block[i] = indexes[0];
        block[i + 1] = indexes[1];
    }
    disk.write(1, block);
}

void FS::readInFatRoot()
{   
    uint8_t block[4096];
    disk.read(1, block);
    for (int i = 0;i < BLOCK_SIZE/2;i += 2)
    {
        fat[i] = fatDiskIndexCalc(block[i], block[i + 1]);
    }
}

// formats the disk, i.e., creates an empty file system
int FS::format()
{
    uint8_t block[4096];
    disk.write(0, block);
    disk.write(1, block);
    fat[0] = -2; //-2 taken, EOF
    fat[1] = -2;
    for (int i = 2; i < BLOCK_SIZE / 2; i++)
    {
        fat[i] = -1; //-1 free
    }
    dir_entry *rootDir = new dir_entry;
    rootDir->file_name[0] = '/';
    rootDir->first_blk = 0;
    rootDir->size = 4096;
    rootDir->type = 1;
    rootDir->access_rights = 0x06;
    entries.push_back(rootDir);

<<<<<<< HEAD
    updateFatRoot();
=======
    /*
    uint8_t fatBlock[4096];
    for (int i = 3;i < 4096;i++)
    {
        fat[i] = 0;
    }
    fatBlock[0] = -1;
    fatBlock[2] = -1;
    disk.write(1, fatBlock);
    */

>>>>>>> db0d1f4a3dd5ef9aca6c856ecc150ca708dd6bf1

    return 0;
}

int FS::getFreeIndex()
{
    for (int i = 0; i < BLOCK_SIZE / 2; i++)
    {
        if (fat[i] == -1)
        {
            return i;
        }
    }

    return -2;
}

void FS::testDisk()
{
    uint8_t recieve[4096];
<<<<<<< HEAD
    disk.read(entries[0]->first_blk, recieve);
    for (int i = 0; i < 4096 && recieve[i] != '\0'; i++)
    {
        std::cout << recieve[i];
    }
    std::cout << std::endl;
=======
    value[0] = 255;
    disk.write(0, value);
    disk.read(0, recieve);
    std::cout << recieve[0] << std::endl;
    std::cout << recieve[0] << std::endl;
>>>>>>> db0d1f4a3dd5ef9aca6c856ecc150ca708dd6bf1
}

// create <filepath> creates a new file on the disk, the data content is
// written on the following rows (ended with an empty row)
int FS::create(std::string filepath)
{
    std::cin.clear();
    std::cout << "FS::create(" << filepath << ")\n";
    std::string contents;
    std::string row = " ";
<<<<<<< HEAD
    uint8_t block[10][4096];
    int firstFatIndex = 0;
    int pred = -2;
    for (int i = 0;i < 5;i++)
    {
        std::cin >> row;
        row.push_back('\n');
=======
    uint8_t block[4096];
    int pred = 0;
    std::cin >> row;
    while (true)
    {
>>>>>>> db0d1f4a3dd5ef9aca6c856ecc150ca708dd6bf1
        contents.append(row);
    }
    int count = 0;
    int index = 0;
    for (int i = 0; i < contents.size(); i++)
    {
        if (count >= 4096)
        {
            count = 0;
            for (int j = 0; j < 4096; j++)
            {
                block[0][j] = 0;
            }
            std::cout << "Increased index\n";
            index++;
        }
        block[0][count] = contents[i];
        count++;
    }
    for (int i = 0; i <= index; i++)
    {
        int fatIndex = getFreeIndex();
        if (pred != -2)
        {
            fat[pred] = fatIndex;
        }
        else
        {
            firstFatIndex = fatIndex;
            fat[fatIndex] = pred;
        }
        pred = fatIndex;
        disk.write(fatIndex, block[i]);
    }

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

    return 0;
}

// cat <filepath> reads the content of a file and prints it on the screen
int FS::cat(std::string filepath)
{
    std::cout << "FS::cat(" << filepath << ")\n";
    return 0;
}

// ls lists the content in the currect directory (files and sub-directories)
int FS::ls()
{
    std::cout << "FS::ls()\n";
    return 0;
}

// cp <sourcepath> <destpath> makes an exact copy of the file
// <sourcepath> to a new file <destpath>
int FS::cp(std::string sourcepath, std::string destpath)
{
    std::cout << "FS::cp(" << sourcepath << "," << destpath << ")\n";
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
