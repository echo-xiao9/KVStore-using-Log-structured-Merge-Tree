#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <iostream>
#include <vector>
#include <fstream>
#include "SSTable.h"
#include "const.h"
using namespace::std;


struct Node {
    Node *right,*down;
    uint64_t key;
    string val;
    Node(Node *right, Node *down, uint64_t key, std::string val):
        right(right), down(down), key(key), val(val){}
    Node(): right(nullptr), down(nullptr) {}
};

class SkipList {
private:
    Node *head;
    uint64_t size;     //number of key
    
public:
    SkipList();
    SkipList(std::vector<KeyVal> keyVals);
    ~SkipList();

    uint64_t getSize(){return size;}
    uint64_t memSize; //the size after convertign to sstable

    /* insert key and value */
    bool put(const uint64_t& key, const std::string& val);
    std::string* get(const uint64_t& key);
    void init();
    bool remove(const uint64_t& key);
    bool del(const uint64_t& key);

    /* save to SSTable */
    MemInfo *generateSSTable(const string &dir, const uint64_t &currentTime);
};


#endif // SKIPLIST_H
