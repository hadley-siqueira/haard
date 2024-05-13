#include "ast/pointer_type.h"

using namespace haard;

PointerType::PointerType(Type* subtype) {
    set_kind(TYPE_POINTER);
    set_subtype(subtype);
}

std::string PointerType::to_cpp() {
    return get_subtype()->to_cpp() + "*";
}
