#ifndef HAARD_SUBTYPED_TYPE_H
#define HAARD_SUBTYPED_TYPE_H

#include "ast/types/type.h"

namespace haard {
    class SubtypedType : public Type {
    public:
        SubtypedType();
        SubtypedType(int kind, Type* type);

    public:
        Type* get_subtype() const;
        void set_subtype(Type *newSubtype);

    private:
        Type* subtype;
    };
}
#endif
