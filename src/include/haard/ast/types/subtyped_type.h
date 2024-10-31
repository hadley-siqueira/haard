#ifndef HAARD_AST_SUBTYPED_TYPE_H
#define HAARD_AST_SUBTYPED_TYPE_H

#include "haard/ast/types/type.h"

namespace haard {
    class SubtypedType : public Type {
    public:
        SubtypedType();
        SubtypedType(AstKind kind);
        ~SubtypedType();

    public:

        Type* get_subtype() const;
        void set_subtype(Type* type);

    private:
        Type* subtype;
    };
}

#endif
