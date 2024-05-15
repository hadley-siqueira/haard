#include "ast/types/reference_type.h"

using namespace haard;

ReferenceType::ReferenceType(Type* subtype) {
    set_kind(TYPE_REFERENCE);
    set_subtype(subtype);
}
