#ifndef HAARD_PARSER_H
#define HAARD_PARSER_H

#include <vector>
#include <stack>
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

            Ast* parse_function();

            Ast* parse_type_parameters();

            Ast* parse_identifier();

        private:
            bool match(TokenKind kind);
            bool lookahead(TokenKind kind);
            void advance();
            bool has_next();

            void indent(unsigned int level);
            void dedent();
            bool is_indented();

        private:
            Token matched;
            std::vector<Token> tokens;
            std::stack<unsigned int> indentation_stack;
            size_t current_token_idx;

    };
}

#endif
