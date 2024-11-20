#include "haard/ast/expressions/operators/unary/pos_increment.h"

using namespace haard;

PosIncrement::PosIncrement() {
    set_kind(AST_POS_INCREMENT);
}

PosIncrement::PosIncrement(Token& token, Expression* expression) {
    set_kind(AST_POS_INCREMENT);
    set_token(token);
    set_expression(expression);
}