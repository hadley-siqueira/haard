#include "haard/ast/expressions/operators/binary/times_assignment.h"

using namespace haard;

TimesAssignment::TimesAssignment() {
    set_kind(AST_TIMES_ASSIGNMENT);
}

TimesAssignment::TimesAssignment(Token& token, Expression* left, Expression* right) {
    set_kind(AST_TIMES_ASSIGNMENT);
    set_token(token);
    set_left(left);
    set_right(right);
}