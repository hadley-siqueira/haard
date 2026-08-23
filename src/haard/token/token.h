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
        TK_AND,
        TK_OR,
        TK_RETURN,
        TK_CONTINUE,
        TK_BREAK,
        TK_YIELD,
        TK_THIS,
        TK_NULL,
        TK_NEW,
        TK_DELETE,
        TK_SIZEOF,

        // the builtin types, the only type names that are words of their
        // own. Everything else a type may name is an identifier
        TK_U8,
        TK_U16,
        TK_U32,
        TK_U64,
        TK_I8,
        TK_I16,
        TK_I32,
        TK_I64,
        TK_F32,
        TK_F64,
        TK_BOOL,
        TK_VOID,
        TK_CHAR,
        TK_PASS,
        TK_GOTO,
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
        TK_ASSIGNMENT,
        TK_PLUS_ASSIGNMENT,
        TK_MINUS_ASSIGNMENT,
        TK_TIMES_ASSIGNMENT,
        TK_DIVISION_ASSIGNMENT,
        TK_INTEGER_DIVISION_ASSIGNMENT,
        TK_MODULO_ASSIGNMENT,
        TK_BITWISE_AND_ASSIGNMENT,
        TK_BITWISE_OR_ASSIGNMENT,
        TK_BITWISE_XOR_ASSIGNMENT,
        TK_BITWISE_NOT_ASSIGNMENT,
        TK_BITWISE_LEFT_SHIFT_ASSIGNMENT,
        TK_BITWISE_RIGHT_SHIFT_ASSIGNMENT,
        TK_BITWISE_UNSIGNED_RIGHT_SHIFT_ASSIGNMENT,
        TK_PLUS,
        TK_MINUS,
        TK_TIMES,
        TK_POWER,
        TK_DIVISION,
        TK_INTEGER_DIVISION,
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
        TK_BITWISE_UNSIGNED_RIGHT_SHIFT,
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

            // most significant bit of 'flags': set when a line break separates
            // this token from the previous one. It is computed from the line
            // the previous token *ended* on, so the operator right after a
            // multiline string does not look like the start of a new line.
            // It answers only about neighbours: for an arbitrary pair of
            // tokens use SourceFile::position_of instead of this bit
            void set_newline_before(bool value);
            bool get_newline_before();

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
