#include "haard/ast/expressions/operators/binary/integer_division_assignment.h"

using namespace haard;

IntegerDivisionAssignment::IntegerDivisionAssignment() {
    set_kind(AST_INTEGER_DIVISION_ASSIGNMENT);
}

IntegerDivisionAssignment::IntegerDivisionAssignment(Token& token, Expression* left, Expression* right) {
    set_kind(AST_INTEGER_DIVISION_ASSIGNMENT);
    set_token(token);
    set_left(left);
    set_right(right);
}