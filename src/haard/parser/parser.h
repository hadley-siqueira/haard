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
            u32 parse_module();
            u32 parse_import();
            u32 parse_import_path();
            u32 parse_import_path_member();
            u32 parse_import_alias();

            u32 parse_let_declaration();
            u32 parse_const_declaration();

            u32 parse_binding();
            u32 parse_binding_name();
            u32 parse_binding_type();
            u32 parse_binding_expression();

            u32 parse_type();
            u32 parse_expression();

            u32 parse_identifier();

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
