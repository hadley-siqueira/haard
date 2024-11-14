#include "haard/ast/expressions/operators/binary/power.h"

using namespace haard;

Power::Power() {
    set_kind(AST_POWER);
}

Power::Power(Token& token, Expression* left, Expression* right) {
    set_kind(AST_POWER);
    set_token(token);
    set_left(left);
    set_left(right);
}