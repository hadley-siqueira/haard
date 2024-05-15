#ifndef HAARD_TYPE_POOL_H
#define HAARD_TYPE_POOL_H

#include <vector>
#include "ast/types/type.h"

namespace haard {
    class TypePool {
    public:
        ~TypePool();

    public:
        void add_type(Type* type);

    private:
        std::vector<Type*> types;
    };
}

#endif
