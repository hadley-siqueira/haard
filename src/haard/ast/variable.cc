#include <haard/ast/variable.h>

using namespace haard;

Variable::Variable() {
    set_kind(AST_VARIABLE);
    set_const(false);
    set_type(nullptr);
    set_expression(nullptr);
}

Variable::~Variable() {
    delete type;
    delete expression;
}

void Variable::set_name(const Token& token) {
    this->name = token;
}

void Variable::set_const(bool value) {
    this->const_flag = value;
}

Token& Variable::get_name() {
    return name;
}

bool Variable::is_const() {
    return const_flag;
}

void Variable::set_type(Type* type) {
    this->type = type;

    if (type) {
        type->set_parent_node(this);
    }
}

Type* Variable::get_type() {
    return type;
}

void Variable::set_expression(Expression* expression) {
    this->expression = expression;

    if (expression) {
        expression->set_parent_node(this);
    }
}

Expression* Variable::get_expression() {
    return expression;
}
