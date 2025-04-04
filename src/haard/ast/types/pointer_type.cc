#include "haard/ast/types/pointer_type.h"

using namespace haard;

PointerType::PointerType() {
    set_kind(AST_TYPE_POINTER);
    set_subtype(nullptr);
}

PointerType::PointerType(Type* subtype, Token& token) {
    set_kind(AST_TYPE_POINTER);
    set_subtype(subtype);
    set_token(token);
}

PointerType::~PointerType() {
    delete subtype;
}

const Token& PointerType::get_token() const {
    return token;
}

void PointerType::set_token(const Token& token) {
    this->token = token;
}

Type* PointerType::get_subtype() const {
    return subtype;
}

void PointerType::set_subtype(Type* subtype) {
    this->subtype = subtype;
}

bool PointerType::equals(Type* other) {
    if (other == nullptr) {
        return false;
    }

    if (get_kind() != other->get_kind()) {
        return false;
    }

    PointerType* o2 = (PointerType*) other;

    return get_subtype()->equals(o2->get_subtype());
}
