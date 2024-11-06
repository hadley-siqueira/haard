#include <sstream>

#include "haard/ast/variable.h"

using namespace haard;

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

std::string Variable::to_json() {
    return "variable json";
}

std::string Variable::to_str() {
    std::stringstream ss;

    if (parent != nullptr) {
        auto kind = parent->get_kind();

        switch (kind) {
        case AST_FUNCTION:
            ss << "@";
            break;

        case AST_COMPOUND_STATEMENT:
        case AST_MODULE:
            ss << "var ";

        default:
            break;
        }
    }

    ss << name.get_value();

    if (type) {
        ss << " : " << type->to_str();
    }

    if (expression) {
        ss << " = " << expression->to_str();
    }

    return ss.str();
}
