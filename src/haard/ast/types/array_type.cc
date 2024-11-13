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

Type* ArrayType::get_subtype() const {
    return subtype;
}

void ArrayType::set_subtype(Type* type) {
    if (type != nullptr) {
        this->subtype = type;
        type->set_parent(this);
    }
}

Ast* ArrayType::get_expression() const {
    return expression;
}

void ArrayType::set_expression(Ast* expression) {
    this->expression = expression;
}
