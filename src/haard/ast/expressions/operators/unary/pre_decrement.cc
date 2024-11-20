#include "haard/ast/expressions/operators/unary/pre_decrement.h"

using namespace haard;

PreDecrement::PreDecrement() {
    set_kind(AST_PRE_DECREMENT);
}

PreDecrement::PreDecrement(Token& token, Expression* expression) {
    set_kind(AST_PRE_DECREMENT);
    set_token(token);
    set_expression(expression);
}