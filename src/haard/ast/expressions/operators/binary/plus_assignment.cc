#include "haard/ast/expressions/operators/binary/plus_assignment.h"

using namespace haard;

PlusAssignment::PlusAssignment() {
    set_kind(AST_PLUS_ASSIGNMENT);
}

PlusAssignment::PlusAssignment(Token& token, Expression* left, Expression* right) {
    set_kind(AST_PLUS_ASSIGNMENT);
    set_token(token);
    set_left(left);
    set_left(right);
}