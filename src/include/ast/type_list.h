#ifndef HAARD_GENERICS_H
#define HAARD_GENERICS_H

#include <vector>
#include "ast/types/type.h"

namespace haard {
    class TypeList : public Type {
    public:
        TypeList(int kind);

    public:
        void add_type(Type* type);
        Type* get_type(int idx);
        int types_count();

    private:
        std::vector<Type*> types;
    };
}

#endif
