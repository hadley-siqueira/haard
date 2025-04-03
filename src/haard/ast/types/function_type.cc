#include <sstream>

#include "haard/ast/types/function_type.h"

using namespace haard;

FunctionType::FunctionType() {
    set_kind(AST_TYPE_FUNCTION);
    set_return_type(nullptr);
}

FunctionType::FunctionType(std::vector<Type*>& parameters_type, Type* return_type) {
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

bool FunctionType::equals(Type* other) {
    if (other == nullptr) {
        return false;
    }

    if (get_kind() != other->get_kind()) {
        return false;
    }

    FunctionType* o2 = (FunctionType*) other;

    if (!o2->get_return_type()->equals(get_return_type())) {
        return false;
    }

    if (get_parameters_type().size() != o2->get_parameters_type().size()) {
        return false;
    }

    int n = get_parameters_type().size();

    for (int i = 0; i < n; ++i) {
        Type* t1 = get_parameters_type()[i];
        Type* t2 = o2->get_parameters_type()[i];

        if (!t1->equals(t2)) {
            return false;
        }
    }

    return true;
}
