#include <haard/ast/function.h>

using namespace haard;

Function::Function() {
    set_kind(AST_FUNCTION);
}

Function::~Function() {
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

void Function::set_name(const Token& token) {
    name = token;
}
