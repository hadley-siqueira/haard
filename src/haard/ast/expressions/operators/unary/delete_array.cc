#include "haard/ast/expressions/operators/unary/delete_array.h"

using namespace haard;

DeleteArray::DeleteArray() {
    set_kind(AST_DELETE_ARRAY);
}

DeleteArray::DeleteArray(Token& token, Expression* expression) {
    set_kind(AST_DELETE_ARRAY);
    set_token(token);
    set_expression(expression);
}