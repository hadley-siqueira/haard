#ifndef HAARD_PARSER_H
#define HAARD_PARSER_H

#include <vector>
#include <stack>

#include <haard/log/logger.h>

#include <haard/ast/ast.h>
#include <haard/ast/import.h>
#include <haard/ast/module.h>
#include <haard/ast/variable.h>
#include <haard/ast/type.h>
#include <haard/ast/expression.h>

namespace haard {
    class Parser {
    public:
        Parser();
        ~Parser();

    public:
        Module* read(const std::string& path);
        void set_logger(Logger* logger);

    public:
        Module* parse_module();
        Import* parse_import();
        Variable* parse_variable();

        Type* parse_type();
        Expression* parse_expression();

    private:
        void advance();
        bool lookahead(TokenKind kind, int offset=0);
        bool match(TokenKind kind);
        bool match_same_line(TokenKind kind);
        bool match();

        bool has_next(int offset=0);
        void indent();
        void dedent();
        bool is_indented();
        bool next_token_on_same_line();

    private:
        void log_error_unexpected_token();
        void log_error_missing_import_path();
        void log_error_missing_import_alias();

    private:
        unsigned idx;
        Token matched;
        std::string path;
        std::vector<Token> tokens;
        std::stack<unsigned> indent_stack;
        Logger* logger;
    };
}

#endif
