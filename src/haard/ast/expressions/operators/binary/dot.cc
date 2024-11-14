#include "haard/ast/expressions/operators/binary/dot.h"

using namespace haard;

Dot::Dot() {
    set_kind(AST_DOT);
}

Dot::Dot(Token& token) {
    set_kind(AST_DOT);
    set_token(token);
}
