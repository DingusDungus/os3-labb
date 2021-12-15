#include <string>
#include <vector>
#include <iostream>
#include <cstdint>

#ifndef FSTREE_H
#define FSTREE_H

struct dir_entry {
    char file_name[56]; // name of the file / sub-directory
    uint32_t size; // size of the file in bytes
    uint16_t first_blk; // index in the FAT for the first block of the file
    uint8_t type; // directory (1) or file (0)
    uint8_t access_rights; // read (0x04), write (0x02), execute (0x01)
};
struct Node {
    dir_entry entry;
    std::vector<Node*> children;
    Node(dir_entry val)
    {
        dir = val;
    }
};

class FSTree
{
private:
    Node* root_node = nullptr;
    int depth = 0;


public:
    FSTree();
    ~FSTree();
    // build up the FSTree from data
    void insert(dir_entry entry);
    void buildFileTree();
};

#endif /* FSTREE_H */
