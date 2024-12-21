#include <sstream>

#include "haard/ast/variable.h"

using namespace haard;

Variable::Variable() {
    set_kind(AST_VARIABLE);
    set_type(nullptr);
    set_expression(nullptr);
}

Variable::~Variable() {
    delete type;
    delete expression;
}

const Token& Variable::get_name() const {
    return name;
}

void Variable::set_name(const Token& name) {
    this->name = name;
}

Type* Variable::get_type() const {
    return type;
}

void Variable::set_type(Type* type) {
    this->type = type;
}

AstNode* Variable::get_expression() const {
    return expression;
}

void Variable::set_expression(AstNode* expression) {
    this->expression = expression;
}
