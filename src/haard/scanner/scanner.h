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
            void get_operator();

        private:
            void start_token();
            void end_token();
            void create_token(TokenKind kind);

            void advance(int steps = 1);
            bool lookahead(char c);
            bool lookahead(char c, int offset);
            bool lookahead(const char* s);

            bool is_alpha(int offset = 0);
            bool is_digit(int offset = 0);
            bool is_binary_digit(int offset = 0);
            bool is_octal_digit(int offset = 0);
            bool is_hex_digit(int offset = 0);
            bool is_alphanum(int offset = 0);
            bool is_operator(int offset = 0);
            std::string get_lexeme_from_token();

        private:
            Tokens* tokens;
            SourceFile* source_file;
            Context* context;
            u32 token_offset;
            u32 token_length;
            int template_counter;
            bool template_flag;
            size_t idx;
    };
}

#endif
