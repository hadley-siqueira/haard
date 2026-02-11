#ifndef HAARD_TOKEN_H
#define HAARD_TOKEN_H

#include <haard/haard.h>

namespace haard {
    typedef enum TokenKind {
        TK_UNKNOWN = 0,
        TK_DEF,
        TK_CLASS,
        TK_ENUM,
        TK_UNION
    } TokenKind;

    class Token {
        public:
            void set_kind(TokenKind kind);
            TokenKind get_kind();

        private:
            u8 kind;
            u8 flags;
            u16 length;
            u32 offset;
    };
};

#endif
