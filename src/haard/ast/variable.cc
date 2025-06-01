#include <haard/ast/variable.h>

using namespace haard;

Variable::Variable() {
    set_kind(AST_VAR);
    set_const(false);
    set_expression(nullptr);
    set_type(nullptr);
}

Variable::~Variable() {
    //delete type;
    //delete expression;
}

void Variable::set_name(const Token& token) {
    this->name = name;
}

void Variable::set_const(bool value) {
    this->const_flag = value;
}

bool Variable::is_const() {
    return const_flag;
}

void Variable::set_type(Type* type) {
    this->type = type;
}

Type* Variable::get_type() {
    return type;
}

void Variable::set_expression(Expression* expression) {
    this->expression = expression;
}

Expression* Variable::get_expression() {
    return expression;
}
