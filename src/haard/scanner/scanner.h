#ifndef HAARD_SCANNER_H
#define HAARD_SCANNER_H

#include <vector>
#include <string>

#include <haard/token/token.h>
#include <haard/string_pool/string_pool.h>

namespace haard {
    class Scanner {
        public:
            Scanner();
            
        public:
            std::vector<Token> get_tokens(const std::string& path);

        private:
            void get_keyword_or_identifier();
            void get_number();

            void read_to_buffer(const std::string& path);
            void reset_state();
            bool has_tokens();
            char advance();

            void create_token();
            void start_token();

            bool is_keyword(const std::string& v);

            bool is_whitespace();
            bool is_newline();
            bool is_alpha();
            bool is_digit();
            bool is_alphanum();
            bool is_hex_digit();
            bool is_binary_digit();
            bool is_octal_digit();

            bool match(char c);
            bool peek(char c);
            bool peek(char first, char last);
            bool peek_ahead(char c, int offset);

        private:
            unsigned int ws;
            unsigned int line;
            unsigned int column;
            unsigned int offset;
            unsigned int idx;
            bool line_start;
            std::vector<Token> tokens;
            std::string buffer;
            std::string value;
            Token token;
    };
}

#endif
