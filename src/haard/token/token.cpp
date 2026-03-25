#include <haard/token/token.h>

using namespace haard;

void Token::set_kind(TokenKind kind) {
    this->kind = kind;
}

TokenKind Token::get_kind() {
    return (TokenKind) kind;
}

void Token::set_offset(u32 offset) {
    this->offset = offset;
}

u32 Token::get_offset() {
    return offset;
}

void Token::set_length(u16 length) {
    this->length = length;
}

u16 Token::get_length() {
    return length;
}

