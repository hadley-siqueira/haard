#include "haard/ast/expressions/operators/unary/pos_decrement.h"

using namespace haard;

PosDecrement::PosDecrement() {
    set_kind(AST_POS_DECREMENT);
}

PosDecrement::PosDecrement(Token& token, Expression* expression) {
    set_kind(AST_POS_DECREMENT);
    set_token(token);
    set_expression(expression);
}