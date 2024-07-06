#include "ast/binary_operators/index.h"

using namespace haard;

Index::Index(Expression* left, Expression* right) {
    set_kind(AST_INDEX);
    set_left(left);
    set_right(right);
}

Index::Index(Token& token, Expression* left, Expression* right) {
    set_kind(AST_INDEX);
    set_left(left);
    set_right(right);
    set_token(token);
}
