#ifndef HAARD_PARSER_H
#define HAARD_PARSER_H

#include <string>
#include <vector>
#include <stack>
#include "log/logger.h"

#include "haard/ast/ast.h"
#include "haard/ast/module.h"
#include "haard/ast/import.h"

namespace haard {
    class Parser {
    public:
        Parser();

    public:
        Module* read(std::string path, std::string relative_path = "");

    public:
        Module* parse_module();
        Import* parse_import();

        Ast* parse_definitions();

        Ast* parse_user_type();

        Ast* parse_variable();
        Ast* parse_field();

        Ast* parse_variable_definition();

        Function* parse_function();
        Ast* parse_parameters();
        Ast* parse_parameter();

        Ast* parse_statement();
        Ast* parse_while_statement();
        Ast* parse_for_statement();

        Ast* parse_if_statement();
        Ast* parse_elif_statement();
        Ast* parse_else_statement();

        Ast* parse_return_statement();

        Ast* parse_switch_statement();
        Ast* parse_switch_cases();

        Ast* parse_statements();

        Ast* parse_type();
        Ast* parse_function_type();
        Ast* parse_tuple_or_function_type();
        Ast* parse_primary_type();

        Ast* parse_expression();
        Ast* parse_assignment_expression();
        Ast* parse_cast_expression();

        Ast* parse_logical_or_expression();
        Ast* parse_logical_and_expression();

        Ast* parse_equality_expression();
        Ast* parse_relational_expression();

        Ast* parse_range_expression();

        Ast* parse_arith_expression();
        Ast* parse_term_expression();
        Ast* parse_power_expression();

        Ast* parse_bitwise_or_expression();
        Ast* parse_bitwise_xor_expression();
        Ast* parse_bitwise_and_expression();

        Ast* parse_shift_expression();

        Ast* parse_unary_expression();
        Ast* parse_logical_not();
        Ast* parse_not();
        Ast* parse_address_of();
        Ast* parse_dereference();
        Ast* parse_bitwise_not();
        Ast* parse_unary_minus();
        Ast* parse_unary_plus();
        Ast* parse_pre_increment();
        Ast* parse_pre_decrement();
        Ast* parse_sizeof();
        Ast* parse_simple_unary_operator(AstKind ast_type, TokenKind token_type, const char* oper);
        Ast* parse_binary_operator(AstKind kind, const char* oper, Ast* left, Ast* (Parser::*function)(void));

        Ast* parse_postfix_expression();

        Ast* parse_primary_expression();

        Ast* parse_new_expression();
        Ast* parse_delete_expression();
        Ast* parse_parenthesis_or_tuple_or_sequence();
        Ast* parse_list_expression();
        Ast* parse_array_or_hash_expression();
        Ast* parse_hash(Ast* key);

        Ast* parse_lambda();
        Ast* parse_lambda_parameters();

        Ast* parse_argument_list();

        Ast* parse_generic_instantiation();
        Ast* parse_scope();
        Ast* parse_identifier();

        Ast* parse_generics();
        Ast* parse_type_list(AstKind kind);

    private:
        void advance();
        bool lookahead(int kind, int offset=0);
        bool match(int kind);
        bool match_same_line(int kind);
        bool match();
        bool expect(int kind);
        bool has_next(int offset=0);
        void recover();
        void unexpected_token();

        void indent();
        void dedent();
        bool is_indented();

        bool has_parameters();
        bool next_token_on_same_line();

    private:
        Token matched;
        int idx;

        std::vector<Token> tokens;
        std::stack<int> indent_stack;
    };
}

#endif
