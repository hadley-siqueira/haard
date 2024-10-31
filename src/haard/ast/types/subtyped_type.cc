#include "haard/ast/types/subtyped_type.h"

using namespace haard;

SubtypedType::SubtypedType() {
    set_kind(AST_TYPE_POINTER);
    set_subtype(nullptr);
}

SubtypedType::SubtypedType(AstKind kind) {
    set_kind(kind);
    set_subtype(nullptr);
}

SubtypedType::~SubtypedType() {
    delete subtype;
}

Type* SubtypedType::get_subtype() const {
    return subtype;
}

void SubtypedType::set_subtype(Type* type) {
    subtype = type;
}
