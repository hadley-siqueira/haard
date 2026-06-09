#ifndef HAARD_PARSER_H
#define HAARD_PARSER_H

#include <haard/context/context.h>

namespace haard {
    class Parser {
        public:
            Parser();

        public:
            void set_context(Context* context);

        public:
            void parse_file(const std::string& path);
            u32 parse_import();
            u32 parse_import_path();
            u32 parse_import_alias();

        public:
            bool match(TokenKind kind);
            bool lookahead(TokenKind kind);
            void advance();

        private:
            Context* context;
            Tokens* tokens;
            Ast* ast;
            u32 current_token;
            u32 matched;
    };
}

#endif
