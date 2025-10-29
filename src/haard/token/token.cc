#include <haard/token/token.h>

using namespace haard;

void Token::set_kind(TokenKind kind) {
    this->kind = kind;
}

void Token::set_offset(unsigned int offset) {
    this->offset = offset;
}

void Token::set_line(unsigned int line) {
    this->line = line;
}

void Token::set_column(unsigned int column) {
    this->column = column;
}

void Token::set_whitespace(unsigned int whitespace) {
    this->whitespace = whitespace;
}

void Token::set_value(const char* value) {
    this->value = value;
}

TokenKind Token::get_kind() {
    return kind;
}

unsigned int Token::get_offset() {
    return offset;
}

unsigned int Token::get_line() {
    return line;
}

unsigned int Token::get_column() {
    return column;
}

unsigned int Token::get_whitespace() {
    return whitespace;
}

const char* Token::get_value() {
    return value;
}

std::string Token::get_kind_as_string() {
    switch (kind) {
        case TK_DEF:
            return "TK_DEF";

        case TK_CLASS:
            return "TK_CLASS";

        case TK_STRUCT:
            return "TK_STRUCT";

        case TK_ENUM:
            return "TK_ENUM";

        case TK_UNION:
            return "TK_UNION";

        case TK_WHILE:
            return "TK_WHILE";

        case TK_FOR:
            return "TK_FOR";

        case TK_IF:
            return "TK_IF";

        case TK_ELIF:
            return "TK_ELIF";

        case TK_ELSE:
            return "TK_ELSE";

        case TK_RETURN:
            return "TK_RETURN";

        case TK_YIELD:
            return "TK_YIELD";

        case TK_CONTINUE:
            return "TK_CONTINUE";

        case TK_BREAK:
            return "TK_BREAK";

        case TK_IDENTIFIER:
            return "TK_IDENTIFIER";

        case TK_BINARY_INTEGER:
            return "TK_BINARY_INTEGER";

        case TK_OCTAL_INTEGER:
            return "TK_OCTAL_INTEGER";

        case TK_HEXADECIMAL_INTEGER:
            return "TK_HEXADECIMAL_INTEGER";

        case TK_INTEGER:
            return "TK_INTEGER";

        case TK_FLOAT:
            return "TK_FLOAT";

        default:
            return "TK_UNKNOWN";
    }
}
