#include "haard/ast/expressions/operators/unary/pre_increment.h"

using namespace haard;

PreIncrement::PreIncrement() {
    set_kind(AST_PRE_INCREMENT);
}

PreIncrement::PreIncrement(Token& token, Expression* expression) {
    set_kind(AST_PRE_INCREMENT);
    set_token(token);
    set_expression(expression);
}