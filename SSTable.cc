#include "SSTable.h"
#include <fstream>
#include <iostream>
#include "Filter.h"


// find between start and end devide by mid
// √6
int MemInfo::find(const uint64_t &key, int left, int right)
{
    if(left > right)return -1;
    if(left == right) {
        if(indexes[left].key == key)
            return left;
        else
            return -1;
    }
    int mid = (left + right)/ 2;
    if(indexes[mid].key < key)
        return find(key, mid + 1, right);
    else if(indexes[mid].key == key)return mid;
    else return find(key, left, mid - 1);
}

void SSTable::add(const KeyVal &keyVal)
{
    keyVals.push_back(keyVal);
    memSize += keyVal.value.size()+12;
    size++;
}
//√4
MemInfo::MemInfo(const std::string &dir)
{
    path = dir;
    char *filterBuf = new char[10240];
    std::ifstream inFile(dir, std::ios::binary);
    if(!inFile)
        throw "can't open files";
    // read 32B heads
    inFile.read((char*)&head.CurStamp, 8);
    inFile.read((char*)&head.size, 8);
    inFile.read((char*)&head.minKey, 8);
    inFile.read((char*)&head.maxKey, 8);
    // read 10240B bloom filter
    inFile.read(filterBuf, 10240);
    bloomFilter = new Filter(filterBuf);
    
    uint64_t pairSize = head.size;
    char *indexBuf = new char[pairSize * 12];
    // read key and off to indexBuf
    inFile.read(indexBuf, pairSize * 12);
    for(uint32_t i = 0; i < pairSize; ++i) {
        key_t key=*(uint64_t*)(indexBuf + 12*i);
        off_t off=*(uint32_t*)(indexBuf + 12*i + 8);
        indexes.push_back(KeyOff(key,off));
    }

    delete[] filterBuf;
    delete[] indexBuf;
    inFile.close();
}

int MemInfo::get(const uint64_t &key)
{
    if(bloomFilter->include(key))
        return find(key, 0, indexes.size() - 1);
    return -1;
}


SSTable::SSTable(MemInfo *cache)
{
    path = cache->path;
    std::ifstream file(path, std::ios::binary);
    if(!file) {
        printf("Fail to open file %s", path.c_str());
        exit(-1);
    }
    CurStamp = (cache->head).CurStamp;
    size = (cache->head).size;

    // load from files
    file.seekg((cache->indexes)[0].offset);
    for(auto it = (cache->indexes).begin();;) {
        uint64_t key = (*it).key;
        uint32_t offset = (*it).offset;
        std::string val;
        if(++it == (cache->indexes).end()) {
            file >> val;
            keyVals.push_back(KeyVal(key, val));
            break;
        } else {
            uint32_t length = (*it).offset - offset;
            char *buf = new char[length + 1];
            buf[length] = '\0';
            file.read(buf, length);
            val = buf;
            delete[] buf;
            keyVals.push_back(KeyVal(key, val));
        }
    }
    delete cache;
}
void SSTable::getBloomFilter(MemInfo *cache){
    char bloomFilter[10240];
    uint32_t hash[4] = {0};
    memset(bloomFilter, '0', 10240);
    for(auto it=cache->indexes.begin();it!=cache->indexes.end();it++){
        uint64_t key = (*it).key;
        MurmurHash3_x64_128(&key, sizeof(key), 1, hash);
        for(int i=0;i<4;i++) bloomFilter[hash[i] % 10240]='1';
    }
}


MemInfo *SSTable::saveOne(const std::string &dir, const uint64_t &currentTime, const uint64_t &num)
{
    //open one new cache
    MemInfo *cache = new MemInfo;
    char *storage = new char[memSize];
    Filter *filter = cache->bloomFilter;
    // change currentTime!!
    *(uint64_t*)storage = currentTime;
    *(uint64_t*)(storage + 8) = size;
    *(uint64_t*)(storage + 16) = keyVals.front().key;
    (cache->head).size = size;
    (cache->head).CurStamp = currentTime;
    (cache->head).minKey = keyVals.front().key;

    char *index = storage + 10*KB+32;
    off_t offset = 10*KB+32 + size * 12;

    for(auto it = keyVals.begin(); it != keyVals.end(); ++it) {
        filter->add((*it).key);
        // write key and offset
        *(uint64_t*)index = (*it).key;
        index += 8;
        *(uint32_t*)index = offset;
        index += 4;
        (cache->indexes).push_back(KeyOff((*it).key, offset));
        uint32_t strLen = ((*it).value).size();
        uint32_t newOffset = offset + strLen;
        if(newOffset > memSize) {
            throw "storage Overflow!!!\n";
        }
        strncpy(storage + offset, ((*it).value).c_str(), strLen);
        offset = newOffset;
    }

    *(uint64_t*)(storage + 24) = keyVals.back().key;
    (cache->head).maxKey = keyVals.back().key;
    filter->saveToBuffer(storage + 32);
    string filename = dir + "/sstable" + to_string(currentTime) + "-" + std::to_string(num) + ".sst";
    cache->path = filename;
    std::ofstream outFile(filename, ios::binary | ios::out);
    outFile.write(storage, memSize);
    outFile.close();
    delete[] storage;
    return cache;
}

std::vector<MemInfo*> SSTable::save(const std::string &dir, std::map<uint64_t, uint64_t> &CurStampKeyOff)
{
    std::vector<MemInfo*> caches;
    SSTable newTable;
    uint64_t num;
    if(CurStampKeyOff.count(CurStamp)){
        num = CurStampKeyOff[CurStamp];
    } else {
        num = 1;
    }
    while(!keyVals.empty()) {
        if(newTable.memSize + 12 + keyVals.front().value.size() >= maxTableSize) {
            caches.push_back(newTable.saveOne(dir, CurStamp, num++));
            newTable = SSTable();
        }
        newTable.add(keyVals.front());
        keyVals.pop_front();
    }
    if(newTable.size > 0) {
        caches.push_back(newTable.saveOne(dir, CurStamp, num++));
    }
    CurStampKeyOff[CurStamp] = num;
    return caches;
}

