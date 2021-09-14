#include "SkipList.h"

SkipList::SkipList() {
    head = new Node();  //初始化头结点
    size = 0;
    memSize = 32+10*KB;
}

SkipList::SkipList(std::vector<KeyVal> keyVals){
    for(auto it = keyVals.begin(); it != keyVals.end(); ++it)
        put((*it).key, (*it).value);
}

SkipList::~SkipList() {
    init();
    delete head;
}

// √
std::string* SkipList::get(const uint64_t &key) {
    Node *cur = head;
    std::string* val = nullptr;
    while (cur) {
        while (cur->right && cur->right->key < key) {
            cur = cur->right;
        }
        /*如果找到了对应的键值*/
        if (cur->right && cur->right->key == key) {
            val = new std::string(cur->right->val);
            break;
        }
        cur = cur->down;
    }
    return val;
}

// √
bool SkipList::put(const uint64_t& key, const std::string& val) {
    /*返回false代表插入新值，返回true代表覆盖原值*/
    std::vector<Node*> pathList;    //从上至下记录搜索路径
    Node *p = head;
    while(p){
        while(p->right && p->right->key < key){
            p = p->right;
        };
        /*如果已经存在key则用value进行覆盖*/
        if (p->right && p->right->key == key) {
            p = p->right;
            memSize = memSize - (p->val).size() + val.size();
            while (p) {
                p->val = val;
                p = p->down;
            };
            return true;
        };
        pathList.push_back(p);
        p = p->down;
    }
    
    bool insertUp = true;
    Node* downNode= nullptr;
    while(insertUp && pathList.size() > 0){   //从下至上搜索路径回溯，50%概率
        Node *insert = pathList.back();
        pathList.pop_back();
        insert->right = new Node(insert->right, downNode, key, val); //add新结点
        downNode = insert->right;    //把新结点赋值为downNode
        insertUp = (rand() & 1);   //50%概率
    }
    if(insertUp){  //插入新的头结点，加层
        Node *oldHead = head;
        head = new Node();
        head->right = new Node(NULL, downNode, key, val);
        head->down = oldHead;
    }
    
    size++;
    //加一个key(8Byte)、一个offset(4Byte)、一个string
    memSize += 12 + val.size();
    return false;
}

//bool SkipList::remove(const uint64_t& key) {
//    Node *p = head;
//    Node *tmp = nullptr;
//    bool flag = false;
//    while (p) {
//        while (p->right && p->right->key < key) {
//            p = p->right;
//        }
//        /*如果右节点值相等*/
//        if (tmp != nullptr) {
//            while (p->right != tmp)
//                p = p->right;
//        }
//
//        if (p->right && p->right->key == key) {
//            flag = true;
//            tmp = p->right;
//            p->right = tmp->right;
//
//            tmp = tmp->down;
//            if (!head->right && head->down) head = head->down;
//        }
//        p = p->down;
//    }
//    return flag;
//}

// √
bool SkipList::remove(const uint64_t& key){
    bool flag=false;
    Node*cur = head;
    Node *pred,*target,*below ,*headBelow= nullptr;
    //repeat get(), but we need the position of the pred of the top deleted node
    
    while (cur) {
        while(cur->right && cur->right->key < key){
            cur = cur ->right;
        }
        if(cur->right && cur->right->key == key){
            flag=true;
            pred = cur;
            target = cur->right;
            below = target->down;
            while (target) {
                pred->right = target->right; //横向处理
                //寻找下一层的前继
                pred = pred->down;
                while (pred && pred->right && pred->right != below) {
                    pred=pred->right;
                }
                //如果塔高需要降低,考虑全部删完的情况,这时候不能把头给删掉了。
                if(!head->right && head->down){
                    headBelow=head->down;
                    delete head;
                    head = headBelow;
                }
                //纵向处理
                if(target==NULL){
                    cout<<"target is deleted!";
                }
                
                delete target;
                size--;
                target = below;
                if(target && target->down)below = target->down;
                else below = nullptr;
            }
        }
        if(flag==true)return true;
        cur = cur -> down;
    }
    return false;
}


bool SkipList::del(const uint64_t& key) {
    return put(key, "~DELETED~");
}

void SkipList::init() {
    Node*cur=head;
    Node *delHead;
    while(cur){
        while (cur->right) {
            Node* del=cur->right;
            cur->right = del->right;
            delete del;
        }
        delHead=cur;
        cur=cur->down;
        delete delHead;
    }
    head=new Node();
    memSize=32+10*KB;
    size=0;
}
//
//MemInfo* SkipList::generateSSTable(const std::string &dir, const uint64_t &currentTime){
//    MemInfo *cache = new MemInfo;
//    // get the first node in the button layer of skiplist
//    //√
//    Node *node;
//    Node *p = head;
//    while(p->down){
//        p = p->down;
//    }
//    node= p->right;
//    // use one tempBuffer to avoid frequent write.
//    char *tempBuffer = new char[memSize];
//    Filter *filter = cache->bloomFilter;
//
//    char *index = tempBuffer + 10*KB+32;
//    //get first offset
//    uint32_t offset = 10272 + size * 12;
//    while(1) {
//        // save key and offset to index from tempBuffer and update bloom filter
//        filter->add(node->key);
//        // index: key + off
//        *(uint64_t*)index = node->key;
//        index += 8; // can use sizeof !
//        *(uint32_t*)index = offset;
//        index += 4;
//        // push
//        (cache->indexes).push_back(KeyOff(node->key, offset));
//        uint32_t strLen = (node->val).size();
//        uint32_t newOffset = offset + strLen;
//
//        // tempBuffer present the future sstable
//        memcpy(tempBuffer + offset, (node->val).c_str(), strLen);
//        offset = newOffset;
//
//        if(node->right) node = node->right;
//        else break;
//    }
//
//
//    *(uint64_t*)tempBuffer = currentTime;
//    (cache->head).CurStamp = currentTime;
//
//    *(uint64_t*)(tempBuffer + 8) = size;
//    (cache->head).size = size;
//
//    *(uint64_t*)(tempBuffer + 16) = node->key;
//    (cache->head).minKey = node->key;
//
//    // save max to the head
//    *(uint64_t*)(tempBuffer + 24) = node->key;
//    // the skiplist is in order
//    (cache->head).maxKey = node->key;
//    // head 32B
//    // can just write!
//    filter->saveToBuffer(tempBuffer + 32);
//    // can change !
//    std::string filename = dir + "/sstable" + std::to_string(currentTime) + ".sst";
//    cache->path = filename;
//    std::ofstream outFile(filename, std::ios::binary | std::ios::out);
//    outFile.write(tempBuffer, memSize);
//
//    delete[] tempBuffer;
//    outFile.close();
//    return cache;
//
//}

MemInfo* SkipList::generateSSTable(const std::string &dir, const uint64_t &currentTime){
    MemInfo *cache = new MemInfo;
    // get the first node in the button layer of skiplist
    //√
    Node *node;
    Node *p = head;
    while(p->down){
        p = p->down;
    }
    node= p->right;
    
    char *buffer = new char[memSize];
    Filter *filter = cache->bloomFilter;
    *(uint64_t*)buffer = currentTime;
    (cache->head).CurStamp = currentTime;
    
    *(uint64_t*)(buffer + 8) = size;
    (cache->head).size = size;
    
    *(uint64_t*)(buffer + 16) = node->key;
    (cache->head).minKey = node->key;
    
    char *index = buffer + 10*KB +32;
    
    uint32_t offset = 10*KB+32 + size * 12;
    
    for(int i=0;i<size;i++){
        // save key and offset to index from buffer and update bloom filter
        filter->add(node->key);
        // index: key + off
//        *(uint64_t*)index = node->key;
        *index = node->key;
        index += 8; // can use sizeof !
        *(uint32_t*)index = offset;
        index += 4;
        // push
        (cache->indexes).push_back(KeyOff(node->key, offset));
        uint32_t length = (node->val).size();
        uint32_t newOff = offset + length;
        // buffer present the future sstable
        memcpy(buffer + offset, (node->val).c_str(), length);
        offset = newOff;
        
        if(node->right) node = node->right;
        else break;
    }
    // save max to the head
    *(uint64_t*)(buffer + 24) = node->key;
    // the skiplist is in order
    (cache->head).maxKey = node->key;
    // head 32B
    // can just write!
    filter->saveToBuffer(buffer + 32);
    // can change !
    std::string filename = dir + "/" + std::to_string(currentTime) + ".sst";
    cache->path = filename;
    std::ofstream outFile(filename, std::ios::binary | std::ios::out);
    outFile.write(buffer, memSize);
    
    delete[] buffer;
    outFile.close();
    return cache;
    
}
