#include "haard/ast/expressions/operators/binary/named_argument.h"

using namespace haard;

NamedArgument::NamedArgument() {
    set_kind(AST_NAMED_ARGUMENT);
    set_expression(nullptr);
}

NamedArgument::~NamedArgument() {
    delete expression;
}

Token NamedArgument::get_name() const {
    return name;
}

void NamedArgument::set_name(const Token& name) {
    this->name = name;
}

Expression* NamedArgument::get_expression() const {
    return expression;
}

void NamedArgument::set_expression(Expression* expression) {
    this->expression = expression;
}
