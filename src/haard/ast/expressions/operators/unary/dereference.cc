#include "haard/ast/expressions/operators/unary/dereference.h"

using namespace haard;

Dereference::Dereference() {
    set_kind(AST_DEREFERENCE);
}

Dereference::Dereference(Token& token, Expression* expression) {
    set_kind(AST_DEREFERENCE);
    set_token(token);
    set_expression(expression);
}