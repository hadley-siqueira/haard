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
