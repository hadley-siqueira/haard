#ifndef HAARD_TOKEN_H
#define HAARD_TOKEN_H

#include <haard/haard.h>

namespace haard {
    typedef enum TokenKind {
        TK_UNKNOWN = 0,
        TK_DEF,
        TK_CLASS,
        TK_ENUM,
        TK_UNION,
        TK_STRUCT,
        TK_IF,
        TK_ELIF,
        TK_ELSE,
        TK_INTEGER_LITERAL,
        TK_FLOAT_LITERAL,
        TK_IDENTIFIER,
        TK_PLUS,
        TK_MINUS,
        TK_INCREMENT,
        TK_DECREMENT,
    } TokenKind;

    class Token {
        public:
            void set_kind(TokenKind kind);
            TokenKind get_kind();

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
