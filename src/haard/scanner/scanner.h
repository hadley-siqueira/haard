#ifndef HAARD_SCANNER_H
#define HAARD_SCANNER_H

#include <string>
#include <vector>

#include <haard/token/token.h>

namespace haard {
    class Scanner {
    public:
        Scanner();

    public:
        std::vector<Token> read(std::string path);

    private:
        void read_to_buffer(std::string path);

        void get_token();
        void get_keyword_or_identifier();
        void get_number();
        void get_operator();
        void get_symbol();
        void get_single_quote_string();
        void get_double_quote_string();
        void skip_comment();

        bool has_next(int count=0);

        bool lookahead(char c, int offset=0);

        bool is_alpha(int offset=0);
        bool is_digit(int offset=0);
        bool is_alphanum(int offset=0);
        bool is_symbol();
        bool is_operator();

        bool has_base();
        bool is_binary_digit();
        bool is_octal_digit();
        bool is_hex_digit();

        void advance();
        void advance_utf8(int counter);
        void count_leading_whitespace();
        void start_token();
        void create_token(TokenKind kind);

    private:
        unsigned idx;
        unsigned line;
        unsigned column;
        unsigned token_line;
        unsigned token_column;
        unsigned token_offset;
        unsigned whitespace_counter;
        int template_counter;
        bool template_flag;
        std::string value;
        std::string buffer;
        std::vector<Token> tokens;
    };
}
#endif
