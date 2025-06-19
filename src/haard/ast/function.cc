#include <haard/ast/function.h>

using namespace haard;

Function::Function() {
    set_kind(AST_FUNCTION);
    set_return_type(nullptr);
    set_statements(nullptr);
}

Function::~Function() {
    delete return_type;
    delete statements;

    for (auto p : parameters) {
        delete p;
    }
}

void Function::add_parameter(Variable* param) {
    if (param) {
        parameters.push_back(param);
        param->set_parent_node(this);
    }
}

void Function::set_return_type(Type* type) {
    return_type = type;

    if (type) {
        type->set_parent_node(this);
    }
}

Type* Function::get_return_type() {
    return return_type;
}

void Function::set_statements(Statements* statements) {
    if (statements) {
        statements->set_parent_node(this);
        this->statements = statements;
    }
}

Statements* Function::get_statements() {
    return statements;
}

void Function::set_name(const Token& token) {
    name = token;
}
