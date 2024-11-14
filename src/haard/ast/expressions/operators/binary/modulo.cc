#include "haard/ast/expressions/operators/binary/modulo.h"

using namespace haard;

Modulo::Modulo() {
    set_kind(AST_MODULO);
}

Modulo::Modulo(Token& token, Expression* left, Expression* right) {
    set_kind(AST_MODULO);
    set_token(token);
    set_left(left);
    set_left(right);
}