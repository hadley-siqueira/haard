#ifndef HAARD_REFERENCE_TYPE_H
#define HAARD_REFERENCE_TYPE_H

#include "ast/types/subtyped_type.h"

namespace haard {
    class ReferenceType : public SubtypedType {
    public:
        ReferenceType(Type* subtype);
    };
}

#endif
