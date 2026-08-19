#ifndef HAARD_TOKEN_H
#define HAARD_TOKEN_H

#include <haard/haard.h>
#include <string>

namespace haard {
    typedef enum TokenKind {
        TK_UNKNOWN = 0,
        TK_IMPORT,
        TK_DEF,
        TK_CLASS,
        TK_ENUM,
        TK_UNION,
        TK_STRUCT,
        TK_IF,
        TK_ELIF,
        TK_ELSE,
        TK_FOR,
        TK_WHILE,
        TK_LET,
        TK_CONST,
        TK_TRUE,
        TK_FALSE,
        TK_AS,
        TK_IN,
        TK_NOT,
        TK_INTEGER_LITERAL,
        TK_FLOAT_LITERAL,
        TK_STRING_LITERAL,
        TK_CHAR_LITERAL,
        TK_SYMBOL_LITERAL,
        TK_TEMPLATE_STRING_BEGIN,
        TK_TEMPLATE_STRING_CHUNK,
        TK_TEMPLATE_STRING_END,
        TK_INTERPOLATION_BEGIN,
        TK_INTERPOLATION_END,
        TK_IDENTIFIER,
        TK_BEGIN_GENERIC,
        TK_END_GENERIC,
        TK_ASSIGNMENT,
        TK_PLUS_ASSIGNMENT,
        TK_MINUS_ASSIGNMENT,
        TK_TIMES_ASSIGNMENT,
        TK_DIVISION_ASSIGNMENT,
        TK_MODULO_ASSIGNMENT,
        TK_BITWISE_AND_ASSIGNMENT,
        TK_BITWISE_OR_ASSIGNMENT,
        TK_BITWISE_XOR_ASSIGNMENT,
        TK_BITWISE_LEFT_SHIFT_ASSIGNMENT,
        TK_BITWISE_RIGHT_SHIFT_ASSIGNMENT,
        TK_PLUS,
        TK_MINUS,
        TK_TIMES,
        TK_DIVISION,
        TK_MODULO,
        TK_LEFT_PARENTHESIS,
        TK_RIGHT_PARENTHESIS,
        TK_LEFT_SQUARE_BRACKET,
        TK_RIGHT_SQUARE_BRACKET,
        TK_LEFT_CURLY_BRACKET,
        TK_RIGHT_CURLY_BRACKET,
        TK_COMMA,
        TK_SEMICOLON,
        TK_LOGICAL_NOT,
        TK_BITWISE_AND,
        TK_BITWISE_OR,
        TK_BITWISE_NOT,
        TK_BITWISE_XOR,
        TK_BITWISE_LEFT_SHIFT,
        TK_BITWISE_RIGHT_SHIFT,
        TK_LOGICAL_AND,
        TK_LOGICAL_OR,
        TK_DOLLAR,
        TK_QUESTION_MARK,
        TK_EQUAL,
        TK_NOT_EQUAL,
        TK_LESS_THAN,
        TK_LESS_THAN_OR_EQUAL,
        TK_GREATER_THAN,
        TK_GREATER_THAN_OR_EQUAL,
        TK_INCREMENT,
        TK_DECREMENT,
        TK_DOT,
        TK_INCLUSIVE_RANGE,
        TK_EXCLUSIVE_RANGE,
        TK_ARROW,
        TK_COLON,
        TK_SCOPE,
        TK_AT,
        TK_EOF,
    } TokenKind;

    class Token {
        public:
            Token();

        public:
            void set_kind(TokenKind kind);
            TokenKind get_kind();
            std::string get_kind_as_string();

            void set_offset(u32 offset);
            u32 get_offset();

            void set_length(u16 length);
            u16 get_length();

            // most significant bit of 'flags': flipped on every line change,
            // so tokens on the same line share the same value
            void set_whitespace_flag(bool value);
            bool get_whitespace_flag();

            // 7 least significant bits of 'flags': amount of whitespace at the
            // start of the token's line, used to compare indentation
            void set_whitespace(int whitespace);
            u8 get_whitespace();

        private:
            u8 kind;
            u8 flags;
            u16 length;
            u32 offset;
    };
};

#endif
