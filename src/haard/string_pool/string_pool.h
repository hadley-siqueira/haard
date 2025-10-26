#ifndef HAARD_STRING_POOL_H
#define HAARD_STRING_POOL_H

#include <string>

namespace haard {
    class StringPool {
        public:
            static const char* get(const std::string& value);

        private:
    };
}

#endif
