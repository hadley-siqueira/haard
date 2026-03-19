#ifndef HAARD_SCANNER_H
#define HAARD_SCANNER_H

#include <haard/context/context.h>

namespace haard {
    class Scanner {
        public:
            void get_tokens(std::filesystem::path& path);
            void set_context(Context& context);

        public:
            void get_token();
            bool has_next();

        private:
            bool lookahead(char c);
            bool is_alpha(int offset = 0);
            bool is_digit(int offset = 0);

        private:
            Tokens& tokens;
            SourceFile& source_file;
            Context& context;
            size_t idx;
    };
}

#endif
