#include <sstream>

#include <haard/token/token.h>

using namespace haard;

Token::Token() {
    set_kind(TK_UNKNOWN);
    set_line(0);
    set_column(0);
    set_offset(0);
    set_whitespace(0);
    set_value(nullptr);
}

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
        case TK_IMPORT:
            return "TK_IMPORT";

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

        case TK_AS:
            return "TK_AS";

        case TK_AND:
            return "TK_AND";

        case TK_OR:
            return "TK_OR";

        case TK_NOT:
            return "TK_NOT";

        case TK_INC:
            return "TK_INC";

        case TK_DEC:
            return "TK_DEC";

        case TK_EQ:
            return "TK_EQ";

        case TK_NE:
            return "TK_NE";

        case TK_GT:
            return "TK_GT";

        case TK_LT:
            return "TK_LT";

        case TK_LE:
            return "TK_LE";

        case TK_GE:
            return "TK_GE";

        case TK_PLUS:
            return "TK_PLUS";

        case TK_MINUS:
            return "TK_MINUS";

        case TK_TIMES:
            return "TK_TIMES";

        case TK_DIVISION:
            return "TK_DIVISION";

        case TK_POWER:
            return "TK_POWER";

        case TK_BITWISE_XOR:
            return "TK_BITWISE_XOR";

        case TK_BITWISE_AND:
            return "TK_BITWISE_AND";

        case TK_BITWISE_NOT:
            return "TK_BITWISE_NOT";

        case TK_SLL:
            return "TK_SLL";

        case TK_SRL:
            return "TK_SRL";

        case TK_SRA:
            return "TK_SRA";

        case TK_BEGIN_GENERIC:
            return "TK_BEGIN_GENERIC";

        case TK_END_GENERIC:
            return "TK_END_GENERIC";

        case TK_DOT:
            return "TK_DOT";

        case TK_COMMA:
            return "TK_COMMA";

        case TK_SEMICOLON:
            return "TK_SEMICOLON";

        case TK_COLON:
            return "TK_COLON";

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

        case TK_EOF:
            return "TK_EOF";

        default:
            return "TK_UNKNOWN";
    }
}

std::string Token::to_json() {
    std::stringstream ss;

    ss << "{" 
        << "\"kind\": \"" << get_kind_as_string() << "\", "
        << "\"line\": " << line << ", "
        << "\"column\": " << column << ", "
        << "\"offset\": " << offset << ", "
        << "\"whitespace\": " << whitespace;

    if (value) {
        ss << ", \"value\": \"" << value << "\"";
    }

    ss << "}";

    return ss.str();
}
