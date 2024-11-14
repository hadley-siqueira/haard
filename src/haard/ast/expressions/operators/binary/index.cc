#include "haard/ast/expressions/operators/binary/index.h"

using namespace haard;

Index::Index() {
    set_kind(AST_INDEX);
}

Index::Index(Token& token, Expression* left, Expression* right) {
    set_kind(AST_INDEX);
    set_token(token);
    set_left(left);
    set_left(right);
}