#ifndef HAARD_TOKEN_H
#define HAARD_TOKEN_H

namespace haard {
    typedef enum TokenKind {
        TK_IDENTIFIER,
        TK_DEF,
        TK_CLASS,
    } TokenKind;

    class Token {
        public:
            void set_kind(TokenKind kind);
            void set_offset(unsigned int offset);
            void set_line(unsigned int line);
            void set_column(unsigned int column);
            void set_whitespace(unsigned int whitespace);
            void set_value(const char* value);

            TokenKind get_kind();
            unsigned int get_offset();
            unsigned int get_line();
            unsigned int get_column();
            unsigned int get_whitespace();
            const char* get_value();

        private:
            TokenKind kind;
            unsigned int whitespace;
            unsigned int offset;
            unsigned int line;
            unsigned int column;
            const char* value;
    };
}

#endif
