#include "ast/types/subtyped_type.h"

using namespace haard;

SubtypedType::SubtypedType() {
    set_kind(AST_TYPE_UNKNOWN);
}

SubtypedType::SubtypedType(int kind, Type* type) {
    set_kind(kind);
    set_subtype(type);
}

bool SubtypedType::equals(Type *other) {
    SubtypedType* other_subtyped = (SubtypedType*) other;

    if (get_kind() != other->get_kind()) {
        return false;
    }

    return get_subtype()->equals(other_subtyped->get_subtype());
}

Type* SubtypedType::get_subtype() const {
    return subtype;
}

void SubtypedType::set_subtype(Type* newSubtype) {
    subtype = newSubtype;
}
