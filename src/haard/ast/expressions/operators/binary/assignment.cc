#include "haard/ast/expressions/operators/binary/assignment.h"

using namespace haard;

Assignment::Assignment() {
    set_kind(AST_ASSIGNMENT);
}

Assignment::Assignment(Token& token, Expression* left, Expression* right) {
    set_kind(AST_ASSIGNMENT);
    set_token(token);
    set_left(left);
    set_left(right);
}