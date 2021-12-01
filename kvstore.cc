#include "kvstore.h"
#include <string>
#include "math.h"
//other helpful functions
bool compareTime(MemInfo *a, MemInfo *b){
    return (a->head).CurStamp > (b->head).CurStamp;
}

KVStore::KVStore(const std::string &dir): KVStoreAPI(dir)
{
    if(dir[dir.length()] == '/')
        path = dir.substr(0, dir.length() - 1);
    else
        path = dir;
    currentTime = 0;
    // load cache from existed SSTables
    if (utils::dirExists(path)) {
        std::vector<std::string> levelNames;
        int levelNum = scanDir(path, levelNames);
        if (levelNum > 0) {
            for(int i = 0; i < levelNum; ++i) {
                std::string levelName = "level-" + std::to_string(i);
                // check if the level directory exists
                if(std::count(levelNames.begin(), levelNames.end(), levelName) == 1) {
                    cache.push_back(std::vector<MemInfo*>());
                    string levelDir = path + "/" + levelName;
                    vector<string> tableNames;
                    int tableNum = scanDir(levelDir, tableNames);
                    
                    for(int j = 0; j < tableNum; ++j) {
                        MemInfo* curCache = new MemInfo(levelDir + "/" + tableNames[j]);
                        uint64_t curTime = (curCache->head).CurStamp;
                        cache[i].push_back(curCache);
                        if(curTime > currentTime)
                            currentTime = curTime;
                    }
                } else
                    break;
            }
        } else {
            mkdir((path + "/level-0").c_str());
            cache.push_back(std::vector<MemInfo*>());
        }
    } else {
        mkdir(path.c_str());
        mkdir((path + "/level-0").c_str());
        cache.push_back(std::vector<MemInfo*>());
    }
    skpList = new SkipList();
    currentTime ++;
}

KVStore::~KVStore()
{
    if(skpList->getSize() > 0)
        skpList->generateSSTable(path + "/level-0", currentTime++);
    delete skpList;
    uint64_t levelMax = 1;
    uint32_t levelNum = cache.size();
    for(uint32_t i = 0; i < levelNum; ++i) {
        levelMax *= 2;
        if(cache[i].size() > levelMax)
            compactLevel(i);
        else
            break;
    }
    for(auto it1 = cache.begin(); it1 != cache.end(); ++it1) {
        for(auto it2 = (*it1).begin(); it2 != (*it1).end(); ++it2)
            delete (*it2);
    }
}

/**
 * Insert/Update the key-value pair.
 * No return values for simplicity.
 */
void KVStore::put(uint64_t key, const std::string &s)
{
    int newSize=skpList->memSize + s.size() + 12;
    if(newSize < maxTableSize) {
        skpList->put(key, s);
        return;
    }
    cache[0].push_back(skpList->generateSSTable(path + "/level-0", currentTime++));
    // can use init !
    skpList->init();
    uint64_t levelMax = 1;
    uint32_t levelNum = cache.size();
    for(uint32_t i = 0; i < levelNum; ++i) {
        levelMax *= 2;
        if(cache[i].size() > levelMax)
            compactLevel(i);
        else
            break;
    }
    skpList->put(key, s);
}
/**
 * Returns the (string) value of the given key.
 * An empty string indicates not found.
 */
std::string KVStore::get(uint64_t key)
{
    std::string *value = skpList->get(key);
    if(value){
        if(*value == "~DELETED~")
            return "";
        return *value;
    }
    // key is not in skpList
    int levelNum = cache.size();
    for(int i = 0; i < levelNum; ++i) {
        for(auto it = cache[i].begin(); it != cache[i].end(); it++){
            if(key <= ((*it)->head).maxKey && key >= ((*it)->head).minKey) {
                int pos = (*it)->get(key);
                if(pos < 0) continue;
                ifstream file((*it)->path, std::ios::binary);
                if(!file) throw("can't open file");
                
                string val;
                uint32_t length, offset = ((*it)->indexes)[pos].offset;
                file.seekg(offset);
                
                // check if it is the last keyVal
                if((unsigned long)pos == ((*it)->indexes).size() - 1) {
                    file >> val;
                } else {
                    uint32_t nextOffset = ((*it)->indexes)[pos + 1].offset;
                    length = nextOffset - offset;
                    char *result = new char[length + 1];
                    result[length] = '\0';
                    file.read(result, length);
                    val = result;
                    delete[] result;
                }
                file.close();
                if(val == "~DELETED~")
                    return "";
                else
                    return val;
            }
        }
    }
    return "";
}


/**
 * This resets the kvstore. All key-value pairs should be removed,
 * including memtable and all sstables files.
 */
//√0
void KVStore::reset()
{
    skpList->init();
    for(auto it1 = cache.begin(); it1 != cache.end(); ++it1) {
        for(auto it2 = (*it1).begin(); it2 != (*it1).end(); ++it2)
            delete (*it2);
    }
    cache.clear();
    cache.push_back(std::vector<MemInfo*>());
    vector<string> dirFiles;
    
    int layer=1;
    while (layer<50) {
        //check if the dir exist
        string dirName=getDirName(layer);
        dirFiles.clear();
        if(dirExists(dirName)){
            scanDir(dirName, dirFiles);
            for(int i=0;i<dirFiles.size();i++){
                string fileDir =dirName+"/"+dirFiles[i];
                rmfile(fileDir.c_str());
            }
            utils::rmdir(dirName.c_str());
        }
        layer++;
    }
    mkdir((path+"/level-0").c_str());
}

/**
 * Delete the given key-value pair if it exists.
 * Returns false if the key is not found.
 */
bool KVStore::del(uint64_t key)
{
    string tmp = get(key);
    if(tmp == "")
        return false;
    put(key, "~DELETED~");
    return true;
}



void KVStore::writeToFile(SSTable &st,int layer){
    ofstream outfile;int numOfKey=0;
    int layerFilesIndex[20];
    char curString1[max_len];
    MemInfo *cache;
    string curFileDir1 = getFileName(layer, layerFilesIndex[layer]);
    layerFilesIndex[layer]++;
    try {
//        string dirPath=fileDir+"/level-"+to_string(layer);
        string dirPath=getDirName(layer);
        if(!dirExists(dirPath.c_str())){
            if(mkdir(dirPath.c_str())!=0)
                throw "can't make direct correctly!";
        }
        outfile.open(curFileDir1,ios::out|ios::binary);
        if(!outfile)
            throw "can't open the file!";
    } catch (string s) {
        cout<<"error in write to file function!"<<endl;
        cout<<s;
        return;
    }
    outfile.write((char*)&st.CurStamp, 8);
//    outfile.write((char*)&st.keyVals.size(), 8);
//    outfile.write((char*)&st.keyVals.begin(), 8);
//    outfile.write((char*)&st.keyVals[st.keyVals.size()-1], 8);
    // write bloom filter
    for (int i=0; i<10240; i++) {
        st.getBloomFilter(cache);
//        outfile.write(st.bloomFilter+i,1);
    }
    
    // write key and offset
//    for(auto it = st.keyOff.begin(); it != st.keyOff.end(); it++){
//        outfile.write((char*)(&it->first),sizeof(it->first));
//        outfile.write((char*)(&it->second),sizeof(it->second));
//        numOfKey++;
//
//    }
    for(int i=0;i<st.keyVals.size()-1;i++){
        string curString="";
        memset(curString1, '\0', max_len);
        //        char* curString1=new char[curString.length()];
        strcpy(curString1,curString.c_str());
        outfile.write((char*)curString1, curString.length());
    }
    memset(curString1, '\0', max_len);
//    string curString2=st.data[st.data.size()-1];
//    strcpy(curString1,curString2.c_str());
    outfile.write(curString1, sizeof(curString1));
    outfile.close();
}


void KVStore::merge(std::vector<SSTable> &tables){
    uint32_t tableSize = tables.size();
    if(tableSize == 1)
        return;
    std::vector<SSTable> next;
    for(uint32_t i = 0; i < tableSize/2; ++i) {
        next.push_back(mergeTables(tables[2*i], tables[2*i + 1]));
    }
    if(tableSize % 2 == 1)
        next.push_back(tables[tableSize - 1]);
    merge(next);
    tables = next;
}

// √3
SSTable KVStore::mergeTables(SSTable &a, SSTable &b){
    SSTable newTable;
    KeyVal keyVal;
    newTable.CurStamp = a.CurStamp;
    int i=0,j=0;
    auto itora=a.keyVals.begin();
    auto itorb=b.keyVals.begin();
    while(itora!=a.keyVals.end() &&itorb!= b.keyVals.end()){
        if((*itora).key <(*itorb).key ){
            newTable.add(*itora);
            itora++;
        }else if((*itora).key >(*itorb).key ){
            newTable.add(*itorb);
            itorb++;
        }else{
            newTable.add(*itora);
            itora++;
            itorb++;
        }
    }
    while (itora!=a.keyVals.end()) {
        newTable.add(*itora);
        itora++;
    }
    while (itorb!=b.keyVals.end()) {
        newTable.add(*itorb);
        itorb++;
    }
    return newTable;
}
// √1
void KVStore::compactLevel(uint32_t level){
    std::vector<SSTable> tableToCompact;
    key_t minKey=INT_MAX;
    key_t maxKey=0;
    // find SSTables to compact in this level
    // √11
    // if level==0,compact all the files
    if(level == 0) {
        for(auto it = cache[level].begin(); it != cache[level].end(); ++it) {
            minKey=((*it)->head.minKey<minKey)?(*it)->head.minKey:minKey ;
            maxKey=((*it)->head.maxKey>maxKey)?(*it)->head.maxKey:maxKey ;
            tableToCompact.push_back(SSTable(*it));
        }
        cache[level].clear();
    }
    //compact the left files
    // √12
    else {
        uint64_t limit = pow(2, level+1);
        uint64_t more = cache[level].size() - limit;
        for(int i=0;i<more;i++){
            auto it = cache[level].begin();
            for(uint64_t i = 0; i < cache[level].size() - 1; ++i)
                ++it;
            uint64_t minStampTime = ((*it)->head).CurStamp;  //find the minStampTime
            while(it != cache[level].end() &&  i<more) {
                minKey=((*it)->head.minKey<minKey)?(*it)->head.minKey:minKey ;
                maxKey=((*it)->head.maxKey>maxKey)?(*it)->head.maxKey:maxKey ;
                tableToCompact.push_back(SSTable(*it));
                it = cache[level].erase(it);
            }
        }
    }
    
    // find SSTables to compact in next level
    if(++level < cache.size()) {
        auto it = cache[level].begin();
        while(it != cache[level].end()) {
            key_t nextMin =(*it)->head.minKey;
            key_t nextMax =(*it)->head.maxKey;
            if((!(nextMin> maxKey))&&(!(nextMax < minKey))) {
                tableToCompact.push_back(SSTable(*it));
                it = cache[level].erase(it);
                if(cache[level].size() == 0) break;
            } else ++it;
        }
    } else {
        mkdir((path + "/level-" + std::to_string(level)).c_str());
        cache.push_back(std::vector<MemInfo*>());
    }
    
    for(auto it = tableToCompact.begin(); it != tableToCompact.end(); ++it)
    rmfile((*it).path.c_str());
    merge(tableToCompact);
    std::vector<MemInfo*> newCaches = tableToCompact[0].save(path + "/level-" + std::to_string(level), CurStampKeyOff);
    for(auto it = newCaches.begin(); it != newCaches.end(); ++it) {
        cache[level].push_back(*it);
    }
}



string KVStore::getFileName(int layer, int num){
    string curFileDir = path+"/level-"+to_string(layer)+"/ssTable"+to_string(num)+".sst";
    return curFileDir;
}

string KVStore::getDirName(int layer){
    string curDir = path+"/level-"+to_string(layer);
    return curDir;
}

bool KVStore::fileExist(string fileDir){
    fstream _file;
    _file.open(fileDir, ios::in);
    if (_file){
        _file.seekg(0, ios_base::end);
        fstream::off_type Len = _file.tellg();
        _file.close();
        if (Len == 0)return false;
        else return true;
    }
    _file.close();
    return false;
}

uint64_t KVStore:: GetFileSize(const std::string& file_name){
    std::ifstream in(file_name.c_str());
    in.seekg(0, std::ios::end);
    size_t size = in.tellg();
    in.close();
    return size; //单位是：byte
}

