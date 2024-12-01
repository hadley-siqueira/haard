#include "haard/ast/expressions/operators/unary/delete.h"

using namespace haard;

Delete::Delete() {
    set_kind(AST_DELETE);
}

Delete::Delete(Token& token, Expression* expression) {
    set_kind(AST_DELETE);
    set_token(token);
    set_expression(expression);
}