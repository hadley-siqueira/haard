#include <sstream>

#include "haard/ast/types/function_type.h"

using namespace haard;

FunctionType::FunctionType() {
    set_kind(AST_TYPE_FUNCTION);
    set_return_type(nullptr);
}

FunctionType::FunctionType(std::vector<Type* >& parameters_type, Type* return_type) {
    set_kind(AST_TYPE_FUNCTION);
    set_return_type(return_type);
    set_parameters_type(parameters_type);
}

FunctionType::~FunctionType() {
    delete return_type;

    for (auto t : parameters_type) {
        delete t;
    }
}

const std::vector<Type*>& FunctionType::get_parameters_type() const {
    return parameters_type;
}

void FunctionType::set_parameters_type(const std::vector<Type*>& parameters_type) {
    this->parameters_type = parameters_type;
}

Type* FunctionType::get_return_type() const {
    return return_type;
}

void FunctionType::set_return_type(Type* return_type) {
    this->return_type = return_type;
}

std::string FunctionType::to_json() {
    return "function type json";
}

std::string FunctionType::to_str() {
    std::stringstream ss;

    ss << "(";

    bool first = true;
    for (auto t : parameters_type) {
        if (!first) ss << ", ";
        ss << t->to_str();
        first = false;
    }

    ss << ") -> " << return_type->to_str();
    return ss.str();
}
