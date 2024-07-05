#include "ast/types/reference_type.h"

using namespace haard;

ReferenceType::ReferenceType(Type* subtype) {
    set_kind(AST_TYPE_REFERENCE);
    set_subtype(subtype);
}
