#pragma once

#include "kvstore_api.h"
#include "SkipList.h"
#include "utils.h"
#include <algorithm>
using namespace::utils;

class KVStore : public KVStoreAPI {
	// You can add your implementation here
private:
    string path;
    uint64_t currentTime;

    // 跳表实现内存存储结构
    SkipList *skpList;

    // 分层硬盘存储，SSTable 用于有序地存储多个键值对
    vector<vector<MemInfo*>> cache;

    map<uint64_t, uint64_t> CurStampKeyOff;

    void compactLevel(uint32_t level);
    void writeToFile(SSTable &st,int layer);
    void merge(vector<SSTable> &tables);
    SSTable mergeTables(SSTable &a, SSTable &b);
    string getFileName(int layer, int num);
    string getDirName(int layer);
    bool fileExist(string fileDir);
    uint64_t  GetFileSize(const std::string& file_name);
public:
	KVStore(const string &dir);

	~KVStore();

	void put(uint64_t key, const string &s) override;

	string get(uint64_t key) override;

	bool del(uint64_t key) override;

	void reset() override;

};
