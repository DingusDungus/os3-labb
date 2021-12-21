#include <cstdint>
#include <iostream>
#include <string>
#include "fs.h"

FS::FS()
{
    std::cout << "FS::FS()... Creating file system\n";
    readInFatRoot();
    initTree();
    changeWorkingDir(0);
}

FS::~FS()
{
    updateFat();
    changeWorkingDir(0);
    writeWorkingDirToBlock(0);
    cleanUp();
    delete root;
}

void FS::cleanUpFiles()
{
    for (int i = 0;i < workingDir.size();i++)
    {
        if (workingDir[i]->type == TYPE_FILE)
        {
            delete workingDir[i];
        }
    }
}

void FS::cleanUpDirs(treeNode *branch)
{
    std::cout << branch->entry->file_name << std::endl;
    for (int i = 0;i < branch->children.size();i++)
    {
        cleanUpDirs(branch->children[i]);
    }
    if (branch != root)
    {
        delete branch->entry;
        delete branch;
    }
}

void FS::cleanUp()
{
    cleanUpDirs(root);
    deleteWorkingDir();
}

void FS::deleteWorkingDir()
{
    for (int i = 0; i < workingDir.size(); i++) {
        delete workingDir[i];
    }
}

dir_entry* FS::makeDotDotDir(uint16_t blk)
{
    std::cout << "Wrote directory to blk: " << blk << std::endl;
    dir_entry *dotDotEntry = new dir_entry;
    std::string name = "..";
    name.push_back('\0');
    for (int i = 0; i < 56 && i < name.size(); i++)
    {
        dotDotEntry->file_name[i] = name[i];
    }
    dotDotEntry->first_blk = blk;
    dotDotEntry->size = '-';
    dotDotEntry->access_rights = 0x06;
    dotDotEntry->type = 1;

    return dotDotEntry;
}

int FS::getSecondNum(uint16_t num)
{
    int i;
    for (i = 0; ((i + 1) * 255) < num; i++)
        ;
    return i;
}

uint32_t FS::convert8to32(uint8_t *result)
{
    return (result[0] << 24) | (result[1] << 16) | (result[2] << 8) | result[3];
}

void FS::convert32to8(uint32_t num, uint8_t *result)
{
    result[0] = (num & 0xff000000UL) >> 24;
    result[1] = (num & 0x00ff0000UL) >> 16;
    result[2] = (num & 0x0000ff00UL) >> 8;
    result[3] = (num & 0x000000ffUL);
}

void FS::convert16to8(uint16_t num, uint8_t *result)
{
    result[0] = *((uint8_t *)&(num) + 1);
    result[1] = *((uint8_t *)&(num) + 0);
}

uint16_t FS::convert8to16(uint8_t num1, uint8_t num2)
{
    uint16_t returnVal = (num1 << 8) | num2;
    return returnVal;
}

void FS::findEOF(uint16_t first_blk, uint16_t *result)
{
    int fatIndex = first_blk;
    uint8_t block[4096];
    while (fat[fatIndex] != FAT_EOF)
    {
        fatIndex = fat[fatIndex];
    }
    // Index for the last block
    result[0] = fatIndex;
    disk.read(fatIndex, block);
    // Index where contents end in the last block of the file
    int i;
    for (i = 0; i < 4096 && block[i] != '\0'; i++)
        ;
    result[1] = i;
}

bool FS::dirEmpty(uint16_t blk)
{
    uint8_t block[4096];
    disk.read(blk, block);
    if (block[0] == 0)
    {
        return true;
    }
    return false;
}

void FS::updateFat()
{
    uint8_t block[4096];
    uint8_t bit16[2];
    uint8_t bit32[4];
    for (int i = 0;i < 4096;i++)
    {
        block[i] = 0;
    }
    disk.write(1, block);
    int x = 0;
    // take each FAT array entry and split it into two 8bit (1 byte)
    // workingDir which are placed in the block at index x and x+1
    for (int i = 0; i < BLOCK_SIZE / 2; i++)
    {
        convert16to8(fat[i], bit16);
        block[x] = bit16[0];
        block[x + 1] = bit16[1];
        x += 2;
    }
    // write the FAT block
    disk.write(1, block);
}

void FS::readInFatRoot()
{
    uint8_t block[4096];
    // read the FAT block into block array
    disk.read(1, block);
    // counter
    int x = 0;
    fat[0] = FAT_EOF;
    fat[1] = FAT_EOF;
    // loop through half the size of the FAT block
    // take 2 bytes each iteration and converting them
    // too a 16bit (2 byte) INT and adding it to the FAT array
    // until we have read the whole FAT from file
    for (int i = 2; i < BLOCK_SIZE / 2; i++)
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
        // reset x to point to the first byte of the next dir entry
        int x = i;
        // create a new dir
        newDir = new dir_entry;

        // loop through the 56 bytes of the filename
        // copy it to newDir filename
        for (int i = 0;i < 56;i++)
        {
            newDir->file_name[i] = '\0';
        }
        for (int j = 0; j < 56; j++)
        {
            newDir->file_name[j] = block[x];
            x++;
        }
        // copy the next 4 bytes containing size to reslut array
        for (int j = 0; j < 4; j++)
        {
            result[j] = block[x];
            x++;
        }
        // convert the 4 bytes in result array into a 32bit (4 byte) INT
        // and copy into newDirs size
        newDir->size = convert8to32(result);
        // copy the next 2 bytes containing first_blk into result array
        for (int j = 0; j < 2; j++)
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
        // push the entry into the workingDir array.
        workingDir.push_back(newDir);
    }
}

void FS::initWorkingDir(uint16_t blk)
{
    updateFat();
    cleanUpFiles();
    workingDir.clear();

    uint8_t block[4096];
    // read the dir_entry block into block array
    disk.read(blk, block);
    dir_entry *newDir;
    uint8_t result[4];

    // loop through dir_entry block until we reach end '\0'
    // jumps 64 at each iteration since size of dir_entry is 64 bytes.
    for (int i = 0; i < 4096 && block[i] != '\0'; i += 64)
    {
        // reset x to point to the first byte of the next dir entry
        int x = i;
        // create a new dir
        newDir = new dir_entry();

        // loop through the 56 bytes of the filename
        // copy it to newDir filename
        for (int j = 0; j < 56; j++)
        {
            newDir->file_name[j] = block[x];
            x++;
        }
        // copy the next 4 bytes containing size to reslut array
        for (int j = 0; j < 4; j++)
        {
            result[j] = block[x];
            x++;
        }
        // convert the 4 bytes in result array into a 32bit (4 byte) INT
        // and copy into newDirs size
        newDir->size = convert8to32(result);
        // copy the next 2 bytes containing first_blk into result array
        for (int j = 0; j < 2; j++)
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
        // push the entry into the workingDir array.
        workingDir.push_back(newDir);
    }
}

void FS::changeWorkingDir(uint16_t blk)
{
    updateFat();

    uint8_t block[4096];

    if (blk == ROOT_BLOCK){
        currentNode = root;
    }else if (blk == currentNode->parent->entry->first_blk) {
        currentNode = currentNode->parent;
    }
    else{
        for (int i = 0; i < currentNode->children.size(); i++)
        {
            if (currentNode->children[i]->entry->first_blk == blk)
            {
                currentNode = currentNode->children[i];
                break;
            }
        }
    }

    deleteWorkingDir();
    //cleanUpFiles();
    workingDir.clear();
    // read the dir_entry block into block array
    disk.read(blk, block);
    dir_entry *newDir;
    uint8_t result[4];

    // loop through dir_entry block until we reach end '\0'
    // jumps 64 at each iteration since size of dir_entry is 64 bytes.
    for (int i = 0; i < 4096 && block[i] != '\0'; i += 64)
    {
        // reset x to point to the first byte of the next dir entry
        int x = i;
        // create a new dir
        newDir = new dir_entry();

        // loop through the 56 bytes of the filename
        // copy it to newDir filename
        for (int j = 0; j < 56; j++)
        {
            newDir->file_name[j] = block[x];
            x++;
        }
        // copy the next 4 bytes containing size to reslut array
        for (int j = 0; j < 4; j++)
        {
            result[j] = block[x];
            x++;
        }
        // convert the 4 bytes in result array into a 32bit (4 byte) INT
        // and copy into newDirs size
        newDir->size = convert8to32(result);
        // copy the next 2 bytes containing first_blk into result array
        for (int j = 0; j < 2; j++)
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
        // push the entry into the workingDir array.
        workingDir.push_back(newDir);
    }
}

void FS::initTree()
{
    root = new treeNode;
    root->parent = root;
    dir_entry *newDir = new dir_entry();
    for (int i = 0;i < 56;i++)
    {
        newDir->file_name[i] = '\0';
    }
    newDir->file_name[0] = '/';
    newDir->file_name[1] = '\0';
    newDir->first_blk = ROOT_BLOCK;
    newDir->size = '-';
    newDir->type = TYPE_DIR;
    newDir->access_rights = READ + WRITE;
    root->entry = newDir;
    int size = workingDir.size();
    initTreeContinued(root);
    initWorkingDir(ROOT_BLOCK);
    std::cout << "Ended\n";
    this->currentNode = this->root;
}

void FS::initTreeContinued(treeNode *pBranch)
{
    int size = workingDir.size();
    std::cout << pBranch->entry->file_name << std::endl;

    for (int i = 0; i < size; i++)
    {
        if (workingDir[i]->type == TYPE_DIR && workingDir[i]->file_name != DOTDOT)
        {
            treeNode *newBranch = new treeNode(pBranch, workingDir[i]);
            std::cout << workingDir[i]->file_name << std::endl;

            pBranch->children.push_back(newBranch);

            initWorkingDir(workingDir[i]->first_blk);
            std::cout << std::endl;
            initTreeContinued(newBranch);
            initWorkingDir(newBranch->entry->first_blk);
        }
    }
}

void FS::writeWorkingDirToBlock(uint16_t blk)
{
    uint8_t block[4096];
    uint8_t bit16[2];
    uint8_t bit32[4];

    for (int i = 0; i < 4096; i++)
    {
        block[i] = 0;
    }

    int x = 0;

    // size of a dir_entry is 64 bytes
    for (int i = 0; i < workingDir.size(); i++)
    {
        // loop through file_name char array
        // adding each char into the block array
        for (int j = 0; j < 56; j++)
        {
            block[x] = workingDir[i]->file_name[j];
            x++;
        }
        // convert one 32 bit (4 bytes) INT to four 8bit (1 byte) INTs
        // saved in var "bit32"
        convert32to8(workingDir[i]->size, bit32);
        // add each of the four 8bit (1 byte) INTs to the block.
        for (int j = 0; j < 4; j++)
        {
            block[x] = bit32[j];
            x++;
        }
        // convert 16bit (2 byte) first_blk into two
        // 8bit (1 byte) INTs
        convert16to8(workingDir[i]->first_blk, bit16);
        // add each of the two 8bit (1 byte) INTs to the block.
        for (int j = 0; j < 2; j++)
        {
            block[x] = bit16[j];
            x++;
        }
        // add the type which is already a 8bit (1 byte) INT to the block.
        block[x] = workingDir[i]->type;
        x++;
        // add the access_rights which is already a 8bit (1 byte) INT to the block.
        block[x] = workingDir[i]->access_rights;
        x++;
    }
    // write the dir_entry block
    disk.write(blk, block);

    updateFat();
}

// returns index in workingDir array, -1 if not found
int FS::findIndexWorkingDir(std::string filename)
{
    bool found = false;
    // Tries to find file in workingDir
    int index = -1;
    uint16_t first_blk = 0;
    for (int i = 0; i < workingDir.size(); i++)
    {
        if (workingDir[i]->file_name == filename)
        {
            found = true;
            index = i;
            break;
        }
    }
    // return index
    return index;
}
// return index of first block, -1 if not found.
int FS::findBlockWorkingDir(std::string filename)
{
    bool found = false;
    // Tries to find file in rootblock
    uint16_t first_blk = 0;
    for (int i = 0; i < workingDir.size(); i++)
    {
        if (workingDir[i]->file_name == filename)
        {
            first_blk = workingDir[i]->first_blk;
            found = true;
            break;
        }
    }
    // return index of first block
    if(found){
        return first_blk;
    }
    return -1;
}

bool FS::fileExist(std::string filename)
{
    bool found = false;
    // Tries to find file
    for (int i = 0; i < workingDir.size(); i++)
    {
        if (workingDir[i]->file_name == filename)
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
    changeWorkingDir(0);
    uint8_t block[4096];
    // reset the block array
    for (int i = 0; i < 4096; i++)
    {
        block[i] = 0;
    }
    // overwrite all blocks
    for (int i = 0; i < BLOCK_SIZE / 2; i++) {
        disk.write(i, block);
    }
    fat[ROOT_BLOCK] = FAT_EOF;
    fat[FAT_BLOCK] = FAT_EOF;
    for (int i = 2; i < BLOCK_SIZE / 2; i++)
    {
        fat[i] = FAT_FREE;
    }
    workingDir.clear();

    updateFat();
    currentNode = root;
    initWorkingDir(0);

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
    std::cout << std::endl;
    for (int i = 0; i < workingDir.size(); i++)
    {
        std::cout << workingDir[i]->file_name << " " << workingDir[i]->first_blk << " " << std::to_string(workingDir[i]->type);
        std::cout << std::endl;
    }
    std::cout << "current dir " << currentNode->entry->file_name << " block: " << currentNode->entry->first_blk << std::endl;
    for (int i = 0;i < currentNode->children.size();i++)
    {
        std::cout << currentNode->children[i]->entry->file_name << std::endl;
    }
    std::cout << "Children of: " << currentNode->entry->file_name << std::endl;
    for (int i = 0;i < 10;i++)
    {
        std::cout << fat[i] << " ";
    }
    std::cout << " FAT\n";
}

int FS::writeBlocksFromString(std::string filepath, std::string contents, uint16_t startFatIndex, int blockIndex)
{
    uint8_t block[4096];
    int firstFatIndex = 0;
    int prevIndex = FAT_EOF;

    disk.read(startFatIndex, block);

    // start writing blocks
    int count = blockIndex;
    int fatIndex = 0;
    fatIndex = startFatIndex;
    firstFatIndex = fatIndex;
    for (int i = 0; i < contents.size(); i++)
    {
        // if file is bigger than size of 1 block (4096 bytes)
        if (count > 4095)
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
            // std::cout << "fatIndex: " << fatIndex << std::endl;
            // std::cout << "prevIndex: " << prevIndex << std::endl;
        }
        // add each char from file contents into block.
        block[count] = contents[i];
        count++;
    }

    // write last block
    fat[fatIndex] = FAT_EOF;
    disk.write(fatIndex, block);

    return firstFatIndex;
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
    // add null termination to content
    contents.push_back('\0');

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
            // std::cout << "fatIndex: " << fatIndex << std::endl;
            // std::cout << "prevIndex: " << prevIndex << std::endl;
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

dir_entry* FS::copyDirEntry(dir_entry* dir, std::string name)
{
    // copy over the dir entry
    dir_entry *newEntry = new dir_entry;
    name.push_back('\0');
    for (int i = 0; i < 56 && i < name.size(); i++)
    {
        newEntry->file_name[i] = name[i];
    }
    newEntry->first_blk = dir->first_blk;
    newEntry->size = dir->size;
    newEntry->access_rights = dir->access_rights;
    newEntry->type = dir->type;

    return newEntry;
}

dir_entry* FS::copyDirEntry(dir_entry* dir, std::string name, uint16_t first_blk){
    // copy over the dir entry
    dir_entry *newEntry = new dir_entry;
    name.push_back('\0');
    for (int i = 0; i < 56 && i < name.size(); i++)
    {
        newEntry->file_name[i] = name[i];
    }
    newEntry->first_blk = first_blk;
    newEntry->size = dir->size;
    newEntry->access_rights = dir->access_rights;
    newEntry->type = dir->type;

    return newEntry;
}
// create <filepath> creates a new file on the disk, the data content is
// written on the following rows (ended with an empty row)
int FS::create(std::string filepath)
{

    // throw error if file already exists
    if (fileExist(filepath))
    {
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
    std::cout << "Wrote file to blk: " << firstFatIndex << std::endl;

    dir_entry *newEntry = new dir_entry;
    filepath.push_back('\0');
    for (int i = 0; i < 56 && i < filepath.size(); i++)
    {
        newEntry->file_name[i] = filepath[i];
    }
    newEntry->first_blk = firstFatIndex;
    newEntry->size = contents.size();
    newEntry->access_rights = 0x06;
    newEntry->type = 0;
    workingDir.push_back(newEntry);
    std::cout << "Added file to dir: " << currentNode->entry->file_name << std::endl;

    writeWorkingDirToBlock(currentNode->entry->first_blk);

    return 0;
}

// cat <filepath> reads the content of a file and prints it on the screen
int FS::cat(std::string filepath)
{
    std::cout << "FS::cat(" << filepath << ")\n";
    // Tries to find file in rootblock
    int first_blk = findBlockWorkingDir(filepath);

    // if file cannot be found, throw error.
    if (first_blk == -1)
    {
        return 1;
    }
    int index = findIndexWorkingDir(filepath);
    if (workingDir[index]->type == TYPE_DIR)
    {
        return 2;
    }

    uint8_t block[4096];
    int fatIndex = first_blk;
    while (fatIndex != FAT_EOF && first_blk != 0)
    {
        std::cout << "Current Block Index: " << fatIndex << std::endl;
        disk.read(fatIndex, block);
        for (int i = 0; i < 4096 && block[i] != '\0'; i++)
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
    std::cout << "name\ttype\tsize\n";
    //print files and directories
    for (int i = 0; i < workingDir.size(); i++)
    {
        if (workingDir[i]->type == TYPE_DIR) {
            // print dir
            std::cout
                << workingDir[i]->file_name
                << '\t' << "dir"
                << '\t' << (char)workingDir[i]->size
                << '\n';
        }else {
            // print file
            std::cout
                << workingDir[i]->file_name
                << '\t' << "file"
                << '\t' << workingDir[i]->size
                << '\n';
        }
    }
    return 0;
}

// cp <sourcepath> <destpath> makes an exact copy of the file
// <sourcepath> to a new file <destpath>
int FS::cp(std::string sourcepath, std::string destpath)
{
    std::cout << "FS::cp(" << sourcepath << "," << destpath << ")\n";
    // Tries to find file in rootblock

    uint16_t first_blk = 0;
    uint8_t block[4096];
    int dstEntryIndex = 0;
    int srcEntryIndex = 0;
    std::string contents = "";
    uint8_t destType = 0;
    srcEntryIndex = findIndexWorkingDir(sourcepath);
    dstEntryIndex = findIndexWorkingDir(destpath);
    // Tries to find file in rootblock
    first_blk = findBlockWorkingDir(sourcepath);
    uint16_t destBlk = findBlockWorkingDir(destpath);
    uint16_t prevBlock = currentNode->entry->first_blk;
    // if source file cannot be found, or is a directory throw error.
    if (!fileExist(sourcepath) || workingDir[srcEntryIndex]->type == TYPE_DIR)
    {
        return 1;
    }
    // throw error if destination already exists and is a file
    if (fileExist(destpath) && workingDir[dstEntryIndex]->type == TYPE_FILE)
    {
        return 1;
    }
    // if dest file not found, dont set destType
    if(dstEntryIndex != -1){
        uint8_t destType = workingDir[dstEntryIndex]->type;
    }

    if(destpath == "..")
    {
        // set parents block as destination
        destBlk = currentNode->parent->entry->first_blk;
        // set type as a directory
        destType = 1;
        // set destIndex to -2 to pass exists check.
        dstEntryIndex = -2;
    }

    // read in all the from the sourcefile blocks to contents.
    int fatIndex = first_blk;
    while (fatIndex != FAT_EOF && first_blk != 0)
    {
        // std::cout << "fatIndex: " << fatIndex << std::endl;
        disk.read(fatIndex, block);
        for (int i = 0; i < 4096 && block[i] != '\0'; i++)
        {
            contents.push_back(block[i]);
        }
        fatIndex = fat[fatIndex];
    }

    // if destination exists and is a directory
    if (fileExist(destpath) || dstEntryIndex == -2 && destType == TYPE_DIR)
    {
        // copy to a directory
        // create new file and save its first block. for file to dir copy
        first_blk = writeBlocksFromString(sourcepath, contents);
        // copy over the dir entry, for file to file copy
        dir_entry *newEntry = copyDirEntry(workingDir[srcEntryIndex],sourcepath,first_blk);

        std::cout << "current block block is: " << prevBlock << std::endl;
        std::cout << "destination is block: " << destBlk << std::endl;

        initWorkingDir(destBlk);
        workingDir.push_back(newEntry);
        writeWorkingDirToBlock(destBlk);
        initWorkingDir(prevBlock);
    }
    //otherwise we just copy file in currentDir
    else
    {
        // just copying file in current dir
        // create new file and save its first block. for file to file copy
        first_blk = writeBlocksFromString(destpath, contents);
        // copy over the dir entry, for file to file copy
        dir_entry *newEntry = copyDirEntry(workingDir[srcEntryIndex],destpath,first_blk);

        std::cout << "destination is currentDir" << std::endl;

        workingDir.push_back(newEntry);
    }

    // save to disk
    writeWorkingDirToBlock(currentNode->entry->first_blk);

    return 0;
}

// mv <sourcepath> <destpath> renames the file <sourcepath> to the name <destpath>,
// or moves the file <sourcepath> to the directory <destpath> (if dest is a directory)
int FS::mv(std::string sourcepath, std::string destpath)
{
    std::cout << "FS::mv(" << sourcepath << "," << destpath << ")\n";
    int destIndex = findIndexWorkingDir(destpath);
    int srcIndex = findIndexWorkingDir(sourcepath);
    int destBlock = findBlockWorkingDir(destpath);
    uint16_t srcBlock = currentNode->entry->first_blk;
    uint8_t destType = 0;
    // if dest file not found, dont set destType
    if(destIndex != -1){
        destType = workingDir[destIndex]->type;
    }

    if(destpath == "..")
    {
        // set parents block as destination
        destBlock = currentNode->parent->entry->first_blk;
        // set type as a directory
        destType = 1;
        // set destIndex to -2 to pass exists check.
        destIndex = -2;
    }

    if (destIndex != -1 && srcIndex != -1 && destType == TYPE_DIR)
    {
        // copy over the dir entry, for file to file copy
        std::cout << "Moving file..." << std::endl;
        dir_entry *fileEntry = copyDirEntry(workingDir[srcIndex],sourcepath);

        std::cout << "current block is: " << srcBlock << std::endl;
        std::cout << "destination is block: " << destBlock << std::endl;

        workingDir.erase(workingDir.begin() + srcIndex);
        writeWorkingDirToBlock(srcBlock);
        initWorkingDir(destBlock);
        workingDir.push_back(fileEntry);
        writeWorkingDirToBlock(destBlock);
        initWorkingDir(srcBlock);
    }
    else if (srcIndex != -1 && destIndex == -1)
    {
        std::cout << "Renaming file..." << std::endl;
        int workingDirIndex = findIndexWorkingDir(sourcepath);
        if (workingDirIndex != -1)
        {
            // reset filename to empty
            for (int i = 0; i < 56; i++)
            {
                workingDir[workingDirIndex]->file_name[i] = 0;
            }
            // rename file
            destpath.push_back('\0'); // add null termination to filename
            for (int i = 0; i < destpath.size() && i < 56; i++)
            {
                workingDir[workingDirIndex]->file_name[i] = destpath[i];
            }
        }
    }

    writeWorkingDirToBlock(currentNode->entry->first_blk);

    return 0;
}

// rm <filepath> removes / deletes the file <filepath>
int FS::rm(std::string filepath)
{
    std::cout << "FS::rm(" << filepath << ")\n";
    // if file doesnt exist throw error.
    if (!fileExist(filepath))
    {
        return 1;
    }
    int entryIndex = findIndexWorkingDir(filepath);
    if (workingDir[entryIndex]->type == TYPE_FILE)
    {
        int fatIndex = workingDir[entryIndex]->first_blk;
        int nextIndex = fatIndex;
        while (nextIndex != FAT_EOF && nextIndex != 0)
        {
            std::cout << "Removing block: " << fatIndex << "\n";
            nextIndex = fat[fatIndex];
            fat[fatIndex] = FAT_FREE;
            fatIndex = nextIndex;
        }
        // Erases the dir entry from the vector
        workingDir.erase(workingDir.begin() + entryIndex);
    }
    else
    {
        if (dirEmpty(workingDir[entryIndex]->first_blk))
        {
            fat[workingDir[entryIndex]->first_blk] = FAT_FREE;
            workingDir.erase(workingDir.begin() + entryIndex);
        }
        else
        {
            return 1;
        }
    }

    writeWorkingDirToBlock(currentNode->entry->first_blk);

    return 0;
}

// append <filepath1> <filepath2> appends the contents of file <filepath1> to
// the end of file <filepath2>. The file <filepath1> is unchanged.
int FS::append(std::string filepath1, std::string filepath2)
{
    std::cout << "FS::append(" << filepath1 << "," << filepath2 << ")\n";
    int entryIndex = findIndexWorkingDir(filepath1);
    uint8_t block[4096];

    int fatIndex = workingDir[entryIndex]->first_blk;
    std::string contents;
    // String block
    std::string sBlock;
    // Result array for finding end of destfile both in blocks and inside of block
    uint16_t result[2];
    int i = 0;
    // Reads every block from sourcefile into string
    while (fatIndex != FAT_EOF)
    {
        std::cout << "Iteration"
                  << "\n";
        disk.read(fatIndex, block);
        sBlock = (char *)block;
        contents.append(sBlock);
        fatIndex = fat[fatIndex];
    }
    contents.push_back('\0');
    entryIndex = findIndexWorkingDir(filepath2);

    // Returns last block in file and last index in the block
    findEOF(workingDir[entryIndex]->first_blk, result);
    int count = result[1];
    fatIndex = result[0];
    // Writes string into EOF block at the given position in the last block
    // (i.e., where the contents in the block ends)
    // and onwards into new blocks if needed
    writeBlocksFromString(filepath2, contents, fatIndex, count);
    workingDir[entryIndex]->size += contents.size();

    writeWorkingDirToBlock(currentNode->entry->first_blk);

    return 0;
}

// mkdir <dirpath> creates a new sub-directory with the name <dirpath>
// in the current directory
int FS::mkdir(std::string dirpath)
{
    std::cout << "FS::mkdir(" << dirpath << ")\n";
    int freeIndex = getFreeIndex();
    uint16_t parentBlock = currentNode->entry->first_blk;
    uint8_t block[4096];
    for (int i = 0; i < 4096; i++)
    {
        block[i] = 0;
    }
    disk.write(freeIndex, block);
    fat[freeIndex] = FAT_EOF;
    std::cout << "Wrote directory to blk: " << freeIndex << std::endl;
    // create dir entry
    dir_entry *newEntry = new dir_entry;
    dirpath.push_back('\0');
    for (int i = 0; i < 56 && i < dirpath.size(); i++)
    {
        newEntry->file_name[i] = dirpath[i];
    }
    newEntry->first_blk = freeIndex;
    newEntry->size = '-';
    newEntry->access_rights = 0x06;
    newEntry->type = 1;
    workingDir.push_back(newEntry);
    std::cout << "Added contents to dir: " << dirpath << std::endl;

    // create new treeNode with the new directory
    treeNode *newBranch = new treeNode(currentNode, newEntry);
    currentNode->children.push_back(newBranch);

    // create DOTDOT entry for new directory.
    dir_entry* dotDotDir = makeDotDotDir(parentBlock);

    // write current workingDir
    writeWorkingDirToBlock(currentNode->entry->first_blk);

    // change workingDir to newBranch and add the dotDotDir
    changeWorkingDir(freeIndex);
    workingDir.push_back(dotDotDir);
    writeWorkingDirToBlock(freeIndex);
    // change back to currentDir
    changeWorkingDir(parentBlock);

    return 0;
}

// cd <dirpath> changes the current (working) directory to the directory named <dirpath>
int FS::cd(std::string dirpath)
{

    std::cout << "FS::cd(" << dirpath << ")\n";
    int index = findIndexWorkingDir(dirpath);
    if (index == -1)
    {
        return 1;
    }

    if (workingDir[index]->type == TYPE_DIR)
    {
        std::cout << currentNode->entry->file_name << std::endl;
        changeWorkingDir(workingDir[index]->first_blk);
        std::cout << currentNode->entry->file_name << std::endl;
    }

    return 0;
}

// pwd prints the full path, i.e., from the root directory, to the current
// directory, including the currect directory name
int FS::pwd()
{
    std::cout << "FS::pwd()\n";
    // if we are in root, just print a /
    if (currentNode == currentNode->parent)
    {
        std::cout << '/' << std::endl;
        return 0;
    }
    treeNode *walker = currentNode;
    std::vector<std::string> path;
    while (walker->parent != walker)
    {
        path.push_back(walker->entry->file_name);
        walker = walker->parent;
    }
    for (int i = path.size() - 1; i >= 0; i--)
    {
        std::cout << '/' + path[i];
    }
    std::cout << std::endl;
    return 0;
}

// chmod <accessrights> <filepath> changes the access rights for the
// file <filepath> to <accessrights>.
int FS::chmod(std::string accessrights, std::string filepath)
{
    std::cout << "FS::chmod(" << accessrights << "," << filepath << ")\n";
    return 0;
}
