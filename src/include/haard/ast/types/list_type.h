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

        virtual std::string to_json();
        virtual std::string to_str();

    private:
        Type* subtype;
    };
}

#endif
