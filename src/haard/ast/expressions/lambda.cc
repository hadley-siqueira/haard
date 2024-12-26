#include "haard/ast/expressions/lambda.h"

using namespace haard;

Lambda::Lambda() {
    set_kind(AST_LAMBDA);
    set_return_type(nullptr);
    set_statements(nullptr);
}

Lambda::~Lambda() {
    delete return_type;

    for (auto p : parameters) {
        delete p;
    }
}

void Lambda::add_parameter(Variable* var) {
    if (var) {
        parameters.push_back(var);
        var->set_parent(this);
    }
}

Type* Lambda::get_return_type() const {
    return return_type;
}

void Lambda::set_return_type(Type* return_type) {
    if (return_type) {
        this->return_type = return_type;
        return_type->set_parent(this);
    }
}

std::vector<Variable*>& Lambda::get_parameters() {
    return parameters;
}

void Lambda::set_parameters(const std::vector<Variable*>& parameters) {
    this->parameters = parameters;

    for (auto p : parameters) {
        p->set_parent(this);
    }
}

Statements* Lambda::get_statements() const {
    return statements;
}

void Lambda::set_statements(Statements* statements) {
    if (statements) {
        this->statements = statements;
        statements->set_parent(this);
    }
}
