#include <haard/token/token.h>
#include <unordered_map>

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

std::string Token::get_kind_as_string() {
    static const std::unordered_map<TokenKind, std::string> table = {
        { TK_UNKNOWN, "TK_UNKNOWN" },
        { TK_IMPORT, "TK_IMPORT" },
        { TK_DEF, "TK_DEF" },
        { TK_CLASS, "TK_CLASS" },
        { TK_ENUM, "TK_ENUM" },
        { TK_UNION, "TK_UNION" },
        { TK_STRUCT, "TK_STRUCT" },
        { TK_IF, "TK_IF" },
        { TK_ELIF, "TK_ELIF" },
        { TK_ELSE, "TK_ELSE" },
        { TK_FOR, "TK_FOR" },
        { TK_WHILE, "TK_WHILE" },
        { TK_TRUE, "TK_TRUE" },
        { TK_FALSE, "TK_FALSE" },
        { TK_AS, "TK_AS" },
        { TK_INTEGER_LITERAL, "TK_INTEGER_LITERAL" },
        { TK_FLOAT_LITERAL, "TK_FLOAT_LITERAL" },
        { TK_IDENTIFIER, "TK_IDENTIFIER" },
        { TK_BEGIN_GENERIC, "TK_BEGIN_GENERIC" },
        { TK_END_GENERIC, "TK_END_GENERIC" },
        { TK_ASSIGNMENT, "TK_ASSIGNMENT" },
        { TK_PLUS, "TK_PLUS" },
        { TK_MINUS, "TK_MINUS" },
        { TK_TIMES, "TK_TIMES" },
        { TK_DIVISION, "TK_DIVISION" },
        { TK_MODULO, "TK_MODULO" },
        { TK_EQUAL, "TK_EQUAL" },
        { TK_NOT_EQUAL, "TK_NOT_EQUAL" },
        { TK_LESS_THAN, "TK_LESS_THAN" },
        { TK_LESS_THAN_OR_EQUAL, "TK_LESS_THAN_OR_EQUAL" },
        { TK_GREATER_THAN, "TK_GREATER_THAN" },
        { TK_GREATER_THAN_OR_EQUAL, "TK_GREATER_THAN_OR_EQUAL" },
        { TK_INCREMENT, "TK_INCREMENT" },
        { TK_DECREMENT, "TK_DECREMENT" },
        { TK_DOT, "." },
        { TK_COLON, ":" },
        { TK_AT, "@" },
    };

    return table.at((TokenKind) kind);
}
