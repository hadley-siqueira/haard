#include <sstream>

#include "ast/array_type.h"
#include "cpp_generator/expression_cpp_generator.h"

using namespace haard;

ArrayType::ArrayType(Type* subtype, Expression* expression) {
    set_kind(TYPE_ARRAY);
    set_subtype(subtype);
    set_expression(expression);
}

ArrayType::~ArrayType() {
    delete expression;
}

Expression* ArrayType::get_expression() const {
    return expression;
}

void ArrayType::set_expression(Expression* newExpression) {
    expression = newExpression;
}

std::string ArrayType::to_cpp() {
    ExpressionCppGenerator gen;
    std::stringstream ss;

    ss << get_subtype()->to_cpp() << '[';
    gen.build(get_expression());
    ss << gen.get_output() << ']';

    return ss.str();
}
