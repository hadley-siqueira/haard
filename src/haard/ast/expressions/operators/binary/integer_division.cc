#include "haard/ast/expressions/operators/binary/integer_division.h"

using namespace haard;

IntegerDivision::IntegerDivision() {
    set_kind(AST_INTEGER_DIVISION);
}

IntegerDivision::IntegerDivision(Token& token, Expression* left, Expression* right) {
    set_kind(AST_INTEGER_DIVISION);
    set_token(token);
    set_left(left);
    set_left(right);
}