#include "haard/ast/expressions/operators/unary/unary_minus.h"

using namespace haard;

UnaryMinus::UnaryMinus() {
    set_kind(AST_UNARY_MINUS);
}

UnaryMinus::UnaryMinus(Token& token, Expression* expression) {
    set_kind(AST_UNARY_MINUS);
    set_token(token);
    set_expression(expression);
}