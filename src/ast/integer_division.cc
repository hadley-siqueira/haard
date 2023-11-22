#include "ast/integer_division.h"

using namespace haard;

IntegerDivision::IntegerDivision(Expression* left, Expression* right) {
    set_kind(AST_INTEGER_DIVISION);
    set_left(left);
    set_right(right);
}

IntegerDivision::IntegerDivision(Token& token, Expression* left, Expression* right) {
    set_kind(AST_INTEGER_DIVISION);
    set_left(left);
    set_right(right);
    set_token(token);
}
