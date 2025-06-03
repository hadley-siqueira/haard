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
