#include <iostream>
#include <vector>

struct dir_entry {
    char file_name[56]; // name of the file / sub-directory
    uint32_t size; // size of the file in bytes
    uint16_t first_blk; // index in the FAT for the first block of the file
    uint8_t type; // directory (1) or file (0)
    uint8_t access_rights; // read (0x04), write (0x02), execute (0x01)
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
        this->entry = entry;
    }
};

int main()
{
    dir_entry *newDir = new dir_entry();
    newDir->file_name[0] = '/';
    newDir->first_blk = 0;
    newDir->access_rights = 0x06;
    newDir->size = 0;
    newDir->type = 1;
    treeNode *branch = new treeNode();
    branch->parent = branch;
    branch->entry = newDir;
    treeNode *newBranch;
    for (int i = 0; i < 10;i++)
    {
        newBranch = new treeNode(branch, newDir);
        branch->children.push_back(newBranch);
    }
    std::cout << branch->entry->file_name << std::endl;
    for (int i = 0;i < branch->children.size();i++)
    {
        std::cout << branch->children[i]->entry->file_name << std::endl;
    }
}