#include "haard/ast/expressions/operators/unary/unary_plus.h"

using namespace haard;

UnaryPlus::UnaryPlus() {
    set_kind(AST_UNARY_PLUS);
}

UnaryPlus::UnaryPlus(Token& token, Expression* expression) {
    set_kind(AST_UNARY_PLUS);
    set_token(token);
    set_expression(expression);
}