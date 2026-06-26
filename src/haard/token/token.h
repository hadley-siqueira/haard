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
        TK_TRUE,
        TK_FALSE,
        TK_AS,
        TK_INTEGER_LITERAL,
        TK_FLOAT_LITERAL,
        TK_IDENTIFIER,
        TK_BEGIN_GENERIC,
        TK_END_GENERIC,
        TK_ASSIGNMENT,
        TK_PLUS,
        TK_MINUS,
        TK_TIMES,
        TK_DIVISION,
        TK_MODULO,
        TK_EQUAL,
        TK_NOT_EQUAL,
        TK_LESS_THAN,
        TK_LESS_THAN_OR_EQUAL,
        TK_GREATER_THAN,
        TK_GREATER_THAN_OR_EQUAL,
        TK_INCREMENT,
        TK_DECREMENT,
        TK_DOT,
        TK_COLON,
        TK_AT,
        TK_EOF,
    } TokenKind;

    class Token {
        public:
            void set_kind(TokenKind kind);
            TokenKind get_kind();
            std::string get_kind_as_string();

            void set_offset(u32 offset);
            u32 get_offset();

            void set_length(u16 length);
            u16 get_length();

        private:
            u8 kind;
            u8 flags;
            u16 length;
            u32 offset;
    };
};

#endif
