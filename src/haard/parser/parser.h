#ifndef HAARD_PARSER_H
#define HAARD_PARSER_H

#include <vector>
#include <stack>

#include <haard/log/logger.h>

#include <haard/ast/ast.h>
#include <haard/ast/import.h>
#include <haard/ast/module.h>
#include <haard/ast/variable.h>
#include <haard/ast/function.h>
#include <haard/ast/type.h>
#include <haard/ast/indirection_type.h>
#include <haard/ast/statement.h>
#include <haard/ast/statements.h>
#include <haard/ast/expression_statement.h>
#include <haard/ast/primitive_type.h>
#include <haard/ast/expression.h>
#include <haard/ast/binary_operator.h>
#include <haard/ast/unary_operator.h>
#include <haard/ast/literal.h>

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
        Function* parse_function();
        Variable* parse_parameter();

        Type* parse_type();

        Statement* parse_statement();
        Statements* parse_statements();

        Expression* parse_expression();
        Expression* parse_assignment_expression();
        Expression* parse_arith_expression();
        Expression* parse_postfix_expression();
        Expression* parse_primary_expression();

    private:
        void recover();
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
        void log_error_missing_variable_name(bool const_flag);
        void log_error_missing_type_on_variable_declaration(bool const_flag);

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
