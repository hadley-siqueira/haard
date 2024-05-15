#ifndef HAARD_POINTER_TYPE_H
#define HAARD_POINTER_TYPE_H

#include "ast/types/subtyped_type.h"

namespace haard {
    class PointerType : public SubtypedType {
    public:
        PointerType(Type* subtype);
        std::string to_cpp();
    };
}

#endif
