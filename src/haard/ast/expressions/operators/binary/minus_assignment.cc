#include "haard/ast/expressions/operators/binary/minus_assignment.h"

using namespace haard;

MinusAssignment::MinusAssignment() {
    set_kind(AST_MINUS_ASSIGNMENT);
}

MinusAssignment::MinusAssignment(Token& token, Expression* left, Expression* right) {
    set_kind(AST_MINUS_ASSIGNMENT);
    set_token(token);
    set_left(left);
    set_right(right);
}