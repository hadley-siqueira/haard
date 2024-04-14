#include "ast/variable.h"

using namespace haard;

Variable::Variable() {
    set_type(nullptr);
    set_expression(nullptr);
}

Variable::~Variable() {

}

Type* Variable::get_type() const {
    return type;
}

void Variable::set_type(Type* newType) {
    type = newType;
}

Expression* Variable::get_expression() const {
    return expression;
}

void Variable::set_expression(Expression* newExpression) {
    expression = newExpression;
}
