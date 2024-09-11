#ifndef HAARD_SCANNER2_H
#define HAARD_SCANNER2_H

#include <string>
#include <vector>
#include "token/token.h"
#include "json/json.h"

namespace haard {
    class Scanner2 {
    public:
        Scanner2();

    public:
        Json* read(std::string path);

    private:
        void read_to_buffer(std::string path);

        void get_token();
        void get_keyword_or_identifier();
        void get_number();
        void get_operator();
        void get_single_quote_string();
        void get_double_quote_string();
        void skip_comment();

        bool has_next(int count=0);

        bool lookahead(char c, int offset=0);

        bool is_alpha(int offset=0);
        bool is_digit(int offset=0);
        bool is_alphanum(int offset=0);
        bool is_operator();

        bool has_base();
        bool is_binary_digit();
        bool is_octal_digit();
        bool is_hex_digit();

        void advance();
        void advance_utf8(int counter);
        void count_leading_whitespace();
        void start_token();
        void create_token(int kind);

    private:
        int idx;
        int line;
        int column;
        int token_line;
        int token_column;
        int whitespace_counter;
        int template_counter;
        bool template_flag;
        std::string value;
        std::string buffer;
        Json* tokens;
    };
}
#endif
