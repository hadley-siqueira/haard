#include <haard/token/token.h>
#include <unordered_map>

using namespace haard;

Token::Token() {
    kind = TK_UNKNOWN;
    flags = 0;
    length = 0;
    offset = 0;
}

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

void Token::set_newline_before(bool value) {
    if (value) {
        flags |= 0b10000000;
    } else {
        flags &= 0b01111111;
    }
}

bool Token::get_newline_before() {
    return (flags >> 7) & 1;
}

void Token::set_whitespace(int whitespace) {
    if (whitespace < 0) {
        whitespace = 0;
    } else if (whitespace > 127) {
        whitespace = 127;
    }

    flags = (flags & 0b10000000) | whitespace;
}

u8 Token::get_whitespace() {
    return flags & 0b01111111;
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
        { TK_LET, "TK_LET" },
        { TK_CONST, "TK_CONST" },
        { TK_TRUE, "TK_TRUE" },
        { TK_FALSE, "TK_FALSE" },
        { TK_AS, "TK_AS" },
        { TK_IN, "TK_IN" },
        { TK_NOT, "TK_NOT" },
        { TK_RETURN, "TK_RETURN" },
        { TK_CONTINUE, "TK_CONTINUE" },
        { TK_BREAK, "TK_BREAK" },
        { TK_YIELD, "TK_YIELD" },
        { TK_THIS, "TK_THIS" },
        { TK_PASS, "TK_PASS" },
        { TK_INTEGER_LITERAL, "TK_INTEGER_LITERAL" },
        { TK_FLOAT_LITERAL, "TK_FLOAT_LITERAL" },
        { TK_STRING_LITERAL, "TK_STRING_LITERAL" },
        { TK_CHAR_LITERAL, "TK_CHAR_LITERAL" },
        { TK_SYMBOL_LITERAL, "TK_SYMBOL_LITERAL" },
        { TK_TEMPLATE_STRING_BEGIN, "TK_TEMPLATE_STRING_BEGIN" },
        { TK_TEMPLATE_STRING_CHUNK, "TK_TEMPLATE_STRING_CHUNK" },
        { TK_TEMPLATE_STRING_END, "TK_TEMPLATE_STRING_END" },
        { TK_INTERPOLATION_BEGIN, "TK_INTERPOLATION_BEGIN" },
        { TK_INTERPOLATION_END, "TK_INTERPOLATION_END" },
        { TK_IDENTIFIER, "TK_IDENTIFIER" },
        { TK_ASSIGNMENT, "TK_ASSIGNMENT" },
        { TK_PLUS_ASSIGNMENT, "TK_PLUS_ASSIGNMENT" },
        { TK_MINUS_ASSIGNMENT, "TK_MINUS_ASSIGNMENT" },
        { TK_TIMES_ASSIGNMENT, "TK_TIMES_ASSIGNMENT" },
        { TK_DIVISION_ASSIGNMENT, "TK_DIVISION_ASSIGNMENT" },
        { TK_MODULO_ASSIGNMENT, "TK_MODULO_ASSIGNMENT" },
        { TK_BITWISE_AND_ASSIGNMENT, "TK_BITWISE_AND_ASSIGNMENT" },
        { TK_BITWISE_OR_ASSIGNMENT, "TK_BITWISE_OR_ASSIGNMENT" },
        { TK_BITWISE_XOR_ASSIGNMENT, "TK_BITWISE_XOR_ASSIGNMENT" },
        { TK_BITWISE_LEFT_SHIFT_ASSIGNMENT, "TK_BITWISE_LEFT_SHIFT_ASSIGNMENT" },
        { TK_BITWISE_RIGHT_SHIFT_ASSIGNMENT, "TK_BITWISE_RIGHT_SHIFT_ASSIGNMENT" },
        { TK_PLUS, "TK_PLUS" },
        { TK_MINUS, "TK_MINUS" },
        { TK_TIMES, "TK_TIMES" },
        { TK_DIVISION, "TK_DIVISION" },
        { TK_MODULO, "TK_MODULO" },
        { TK_LEFT_PARENTHESIS, "TK_LEFT_PARENTHESIS" },
        { TK_RIGHT_PARENTHESIS, "TK_RIGHT_PARENTHESIS" },
        { TK_LEFT_SQUARE_BRACKET, "TK_LEFT_SQUARE_BRACKET" },
        { TK_RIGHT_SQUARE_BRACKET, "TK_RIGHT_SQUARE_BRACKET" },
        { TK_LEFT_CURLY_BRACKET, "TK_LEFT_CURLY_BRACKET" },
        { TK_RIGHT_CURLY_BRACKET, "TK_RIGHT_CURLY_BRACKET" },
        { TK_COMMA, "TK_COMMA" },
        { TK_SEMICOLON, "TK_SEMICOLON" },
        { TK_LOGICAL_NOT, "TK_LOGICAL_NOT" },
        { TK_BITWISE_AND, "TK_BITWISE_AND" },
        { TK_BITWISE_OR, "TK_BITWISE_OR" },
        { TK_BITWISE_NOT, "TK_BITWISE_NOT" },
        { TK_BITWISE_XOR, "TK_BITWISE_XOR" },
        { TK_BITWISE_LEFT_SHIFT, "TK_BITWISE_LEFT_SHIFT" },
        { TK_BITWISE_RIGHT_SHIFT, "TK_BITWISE_RIGHT_SHIFT" },
        { TK_LOGICAL_AND, "TK_LOGICAL_AND" },
        { TK_LOGICAL_OR, "TK_LOGICAL_OR" },
        { TK_DOLLAR, "TK_DOLLAR" },
        { TK_QUESTION_MARK, "TK_QUESTION_MARK" },
        { TK_EQUAL, "TK_EQUAL" },
        { TK_NOT_EQUAL, "TK_NOT_EQUAL" },
        { TK_LESS_THAN, "TK_LESS_THAN" },
        { TK_LESS_THAN_OR_EQUAL, "TK_LESS_THAN_OR_EQUAL" },
        { TK_GREATER_THAN, "TK_GREATER_THAN" },
        { TK_GREATER_THAN_OR_EQUAL, "TK_GREATER_THAN_OR_EQUAL" },
        { TK_INCREMENT, "TK_INCREMENT" },
        { TK_DECREMENT, "TK_DECREMENT" },
        { TK_DOT, "TK_DOT" },
        { TK_INCLUSIVE_RANGE, "TK_INCLUSIVE_RANGE" },
        { TK_EXCLUSIVE_RANGE, "TK_EXCLUSIVE_RANGE" },
        { TK_ARROW, "TK_ARROW" },
        { TK_COLON, "TK_COLON" },
        { TK_SCOPE, "TK_SCOPE" },
        { TK_AT, "TK_AT" },
        { TK_EOF, "TK_EOF" },
    };

    return table.at((TokenKind) kind);
}
