#ifndef APP_CSTRINGHASHER_H
#define APP_CSTRINGHASHER_H

#include <boost/multi_index/hashed_index.hpp>

namespace App {
struct CStringHasher {
    inline std::size_t operator()(const char *s) const {
        if(!s) return 0;
        return boost::hash_range(s,s+std::strlen(s));
    }
    inline bool operator()(const char *a, const char *b) const {
        if(!a) return !b;
        if(!b) return false;
        return std::strcmp(a,b)==0;
    }
};
}

#endif