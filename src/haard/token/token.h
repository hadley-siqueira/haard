#ifndef HAARD_TOKEN_H
#define HAARD_TOKEN_H

#include <string>

namespace haard {
    typedef enum TokenKind {
        TK_IMPORT,
        TK_DEF,

        TK_CLASS,
        TK_STRUCT,
        TK_ENUM,
        TK_UNION,

        TK_WHILE,
        TK_FOR,
        TK_IF,
        TK_ELIF,
        TK_ELSE,

        TK_RETURN,
        TK_YIELD,
        TK_CONTINUE,
        TK_BREAK,

        TK_AS,

        TK_AND,
        TK_OR,
        TK_NOT,

        TK_INC,
        TK_DEC,

        TK_EQ,
        TK_NE,
        TK_GT,
        TK_LT,
        TK_LE,
        TK_GE,

        TK_PLUS,
        TK_MINUS,
        TK_TIMES,
        TK_DIVISION,
        TK_POWER,

        TK_BITWISE_XOR,
        TK_BITWISE_AND,
        TK_BITWISE_NOT,

        TK_SLL, 
        TK_SRL,
        TK_SRA,
        TK_BEGIN_GENERIC,
        TK_END_GENERIC,

        TK_DOT,

        TK_COMMA,
        TK_SEMICOLON,
        TK_COLON,

        TK_IDENTIFIER,
        TK_BINARY_INTEGER,
        TK_OCTAL_INTEGER,
        TK_HEXADECIMAL_INTEGER,
        TK_INTEGER,
        TK_FLOAT,
        TK_EOF,
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

            std::string get_kind_as_string();
            std::string to_json();

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
