#include "haard/ast/expressions/operators/binary/division.h"

using namespace haard;

Division::Division() {
    set_kind(AST_DIVISION);
}

Division::Division(Token& token, Expression* left, Expression* right) {
    set_kind(AST_DIVISION);
    set_token(token);
    set_left(left);
    set_right(right);
}