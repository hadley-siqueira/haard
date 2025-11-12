#ifndef HAARD_SCANNER_H
#define HAARD_SCANNER_H

#include <vector>
#include <string>
#include <unordered_map>

#include <haard/token/token.h>
#include <haard/log/logger.h>
#include <haard/string_pool/string_pool.h>

namespace haard {
    class Scanner {
        public:
            Scanner();
            
        public:
            std::vector<Token> get_tokens(const std::string& path);
            const Logger& get_logger();

        private:
            void get_keyword_or_identifier();
            void get_number();
            void get_operator();

            void read_to_buffer(const std::string& path);
            void reset_state();
            bool has_tokens();
            char advance();

            void create_token(TokenKind kind);
            void start_token();

            TokenKind get_keyword_kind(const std::string& v);

            bool is_whitespace();
            bool is_newline();
            bool is_alpha();
            bool is_digit();
            bool is_operator();
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
            unsigned int template_counter;
            bool line_start;
            bool template_flag;
            std::vector<Token> tokens;
            std::unordered_map<std::string, TokenKind> keywords;
            std::unordered_map<std::string, TokenKind> operators;
            std::string buffer;
            std::string value;
            Token token;
            Logger logger;
    };
}

#endif
