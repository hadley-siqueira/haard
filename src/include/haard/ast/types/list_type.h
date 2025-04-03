#ifndef HAARD_AST_LIST_TYPE_H
#define HAARD_AST_LIST_TYPE_H

#include "haard/ast/types/type.h"

namespace haard {
    class ListType : public Type {
    public:
        ListType();
        ListType(Type* subtype);
        ~ListType();

    public:
        Type* get_subtype() const;
        void set_subtype(Type* type);

        bool equals(Type* other);

    private:
        Type* subtype;
    };
}

#endif
