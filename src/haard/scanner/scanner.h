#ifndef HAARD_SCANNER_H
#define HAARD_SCANNER_H

#include <haard/context/context.h>

namespace haard {
    class Scanner {
        public:
            Scanner();

        public:
            void get_tokens(const std::filesystem::path& path);
            void set_context(Context* context);

        public:
            void get_token();
            bool has_next();

        private:
            void get_keyword_or_identifier();
            void get_number();
            void get_string();
            void get_template_string();
            void get_interpolation();
            void get_operator();
            void get_symbol();
            void get_invalid_character();
            void skip_comment();

        private:
            void reset();
            void start_token();
            void end_token();
            void create_token(TokenKind kind);
            void create_eof_token();

            void advance(int steps);
            void advance();
            bool lookahead(char c);
            bool lookahead(char c, int offset);
            bool lookahead(const char* s);

            bool is_newline();
            bool is_whitespace();
            bool is_comment();
            bool is_string();
            bool is_symbol();
            bool is_template_string();
            bool is_interpolation();
            bool is_utf8_continuation(int offset = 0);
            bool is_alpha(int offset = 0);
            bool is_digit(int offset = 0);
            bool is_binary_digit(int offset = 0);
            bool is_octal_digit(int offset = 0);
            bool is_hex_digit(int offset = 0);
            bool is_alphanum(int offset = 0);
            bool is_operator(int offset = 0);
            std::string get_lexeme_from_token();

        private:
            TokenStream* tokens;
            SourceFile* source_file;
            Context* context;
            Logger* logger;
            u32 token_offset;
            u32 token_length;
            u32 token_line;
            u32 token_ws;
            u32 column;
            u32 line;
            u32 ws;
            // the line the previous token ended on, which is what decides the
            // newline flag of the next one
            u32 last_end_line;
            u32 last_tab_line;
            u32 last_deep_indentation_line;
            u32 line_tab_offset;
            bool line_start;
            bool line_has_tab;
            bool token_has_tab;
            size_t idx;
    };
}

#endif
