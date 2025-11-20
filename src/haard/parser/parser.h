#ifndef HAARD_PARSER_H
#define HAARD_PARSER_H

#include <vector>
#include <haard/token/token.h>
#include <haard/ast/ast.h>

namespace haard {
    class Parser {
        public:
            Ast* parse_file(const std::string& path);

        private:
            Ast* parse_module();

            Ast* parse_import();
            Ast* parse_import_path();

            Ast* parse_identifier();

            bool match(TokenKind kind);
            bool lookahead(TokenKind kind);
            void advance();

        private:
            Token matched;
            std::vector<Token> tokens;
            size_t current_token_idx;

    };
}

#endif
