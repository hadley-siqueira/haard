#ifndef HAARD_STRING_POOL_H
#define HAARD_STRING_POOL_H

namespace haard {
    class StringPool {
    public:
        static char* get(const char* value);
    };
}

#endif
