#include <haard/token/token.h>

using namespace haard;

void Token::set_kind(TokenKind kind) {
    this->kind = kind;
}

TokenKind Token::get_kind() {
    return (TokenKind) kind;
}
