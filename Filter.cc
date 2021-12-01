//
//  Filter.cpp
//  LSMLAB
//
//  Created by 康艺潇 on 2021/6/10.
//

#include "Filter.h"
#include "MurmurHash3.h"

#define FILTER_SIZE 81920 // 81920 bit = 10240 B = 10KB
void Filter::add(const uint64_t &key){
    uint32_t hash[4] = {0};
    MurmurHash3_x64_128(&key, sizeof(key), 1, hash);
    for(int i=0;i<4;i++) filter[hash[i] % 10240]='1';
}


bool Filter:: include(const uint64_t &key){
    uint32_t hash[4] = {0};
    MurmurHash3_x64_128(&key, sizeof(key), 1, hash);
    for(int i=0;i<4;i++) {
        if(filter[hash[i] % 10240]!='1')return false;
    }
    return true;
}

void Filter::saveToBuffer(char* buf){
    strncpy(buf, filter, 10240);
}
