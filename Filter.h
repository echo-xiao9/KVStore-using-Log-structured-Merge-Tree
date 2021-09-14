//
//  Filter.hpp
//  LSMLAB
//
//  Created by 康艺潇 on 2021/6/10.
//

#ifndef Filter_h
#define Filter_h

#include <stdio.h>
#include "const.h"
using namespace::std;


class Filter {
public:
    char filter[10240];
    Filter() {memset(filter, '0', 10240);}
    Filter(char *buf){strncpy(filter, buf,10240);}
    void add(const uint64_t &key);
    bool include(const uint64_t &key);
    void saveToBuffer(char* buf);
};



#endif /* Filter_hpp */
