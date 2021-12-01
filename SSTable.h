#ifndef SSTABLE_H
#define SSTABLE_H

#include "Filter.h"
#include <vector>
#include <list>
#include <memory>
#include <map>
#include "utils.h"
#include "MurmurHash3.h"

const static uint64_t maxTableSize = 2097152;

struct KeyVal {
    uint64_t key;
    std::string value;
    KeyVal (uint64_t k = 0, std::string v = std::string()):key(k), value(v) {};
    KeyVal (KeyVal const &e):key(e.key), value(e.value) {}; // copy constructor
};

//head for SSTable
struct Head {
    uint64_t CurStamp; //time stamp
    uint64_t size;      //num of key val
    uint64_t minKey;
    uint64_t maxKey;
    Head(): CurStamp(0), size(0), minKey(0), maxKey(0){}
};

//KeyOffs for SSTable
struct KeyOff {
    uint64_t key;
    uint32_t offset;
    KeyOff(uint64_t k = 0, uint32_t o = 0): key(k), offset(o) {}
};

using namespace::std;
class MemInfo{
public:
    Head head;
    Filter *bloomFilter;
    std::vector<KeyOff> indexes;
    std::string path;

    MemInfo(): bloomFilter(new Filter()) {}
    MemInfo(const std::string &dir);
    ~MemInfo(){delete bloomFilter;}

    int get(const uint64_t &key);
    int find(const uint64_t &key, int left, int right);
};

class SSTable{
public:
    uint64_t CurStamp;
    std::string path;
    uint64_t size;
    uint64_t memSize;
    std::list<KeyVal> keyVals;

    std::vector<MemInfo*> save(const std::string &dir, std::map<uint64_t, uint64_t> &CurStampKeyOff);
    MemInfo *saveOne(const std::string &dir, const uint64_t &currentTime, const uint64_t &num);
    
    SSTable(MemInfo *cache);
    SSTable(): size(0), memSize(10272) {}

    void add(const KeyVal &keyVal);
    void getBloomFilter(MemInfo *cache);
};

#endif // SSTABLE_H
