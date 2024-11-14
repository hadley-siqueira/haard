#include "haard/ast/expressions/operators/binary/times.h"

using namespace haard;

Times::Times() {
    set_kind(AST_TIMES);
}

Times::Times(Token& token, Expression* left, Expression* right) {
    set_kind(AST_TIMES);
    set_token(token);
    set_left(left);
    set_left(right);
}