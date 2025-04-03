#include "haard/ast/types/reference_type.h"

using namespace haard;

ReferenceType::ReferenceType() {
    set_kind(AST_TYPE_REFERENCE);
    set_subtype(nullptr);
}

ReferenceType::ReferenceType(Type* subtype, Token& token) {
    set_kind(AST_TYPE_REFERENCE);
    set_subtype(subtype);
    set_token(token);
}

ReferenceType::~ReferenceType() {
    delete subtype;
}

const Token& ReferenceType::get_token() const {
    return token;
}

void ReferenceType::set_token(const Token& token) {
    this->token = token;
}

Type* ReferenceType::get_subtype() const {
    return subtype;
}

void ReferenceType::set_subtype(Type* subtype) {
    this->subtype = subtype;
}

bool ReferenceType::equals(Type* other) {
    if (other == nullptr) {
        return false;
    }

    if (get_kind() != other->get_kind()) {
        return false;
    }

    ReferenceType* o2 = (ReferenceType*) other;

    return get_subtype()->equals(o2->get_subtype());
}
