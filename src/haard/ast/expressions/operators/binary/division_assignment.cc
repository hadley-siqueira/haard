#include "haard/ast/expressions/operators/binary/division_assignment.h"

using namespace haard;

DivisionAssignment::DivisionAssignment() {
    set_kind(AST_DIVISION_ASSIGNMENT);
}

DivisionAssignment::DivisionAssignment(Token& token, Expression* left, Expression* right) {
    set_kind(AST_DIVISION_ASSIGNMENT);
    set_token(token);
    set_left(left);
    set_left(right);
}