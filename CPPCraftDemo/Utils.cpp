#include "stdafx.h"

namespace core {

std::string genRndStr(int32_t len) {
    static constexpr const char alphanum[] = 
        "0123456789" 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ" 
        "abcdefghijklmnopqrstuvwxyz";
    std::string tmp; tmp.reserve(len);

    for (int32_t i = 0; i < len; ++i) {
        tmp += alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    return tmp;
}

int32_t genRndInt32(int32_t min, int32_t max) {
    return min + (rand() % (max - min + 1));
}

int64_t genRndInt64(int64_t min, int64_t max) {
    return min + (rand() % (max - min + 1));
}

} // namspace core

