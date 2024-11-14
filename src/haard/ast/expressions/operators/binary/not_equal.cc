#include "haard/ast/expressions/operators/binary/not_equal.h"

using namespace haard;

NotEqual::NotEqual() {
    set_kind(AST_NOT_EQUAL);
}

NotEqual::NotEqual(Token& token, Expression* left, Expression* right) {
    set_kind(AST_NOT_EQUAL);
    set_token(token);
    set_left(left);
    set_left(right);
}