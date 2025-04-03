#include <sstream>

#include "haard/ast/types/array_type.h"

using namespace haard;

ArrayType::ArrayType() {
    set_kind(AST_TYPE_ARRAY);
    set_subtype(nullptr);
    set_expression(nullptr);
}

ArrayType::ArrayType(Type* subtype, Token& token) {
    set_kind(AST_TYPE_ARRAY);
    set_subtype(subtype);
    set_token(token);
    set_expression(nullptr);
}

ArrayType::~ArrayType() {
    delete subtype;
    delete expression;
}

const Token& ArrayType::get_token() const {
    return token;
}

void ArrayType::set_token(const Token& token) {
    this->token = token;
}

bool ArrayType::equals(Type* other) {
    if (other == nullptr) {
        return false;
    }

    if (get_kind() != other->get_kind()) {
        return false;
    }

    ArrayType* o2 = (ArrayType*) other;

    return o2->get_subtype()->equals(get_subtype());
}

Type* ArrayType::get_subtype() const {
    return subtype;
}

void ArrayType::set_subtype(Type* type) {
    if (type != nullptr) {
        this->subtype = type;
        type->set_parent(this);
    }
}

AstNode* ArrayType::get_expression() const {
    return expression;
}

void ArrayType::set_expression(AstNode* expression) {
    this->expression = expression;
}
