#ifndef HAARD_PARSER_H
#define HAARD_PARSER_H

#include <string>
#include <vector>
#include <stack>

#include "haard/ast/ast.h"

namespace haard {
    class Parser {
    public:
        Parser();

    public:
        Module* read(std::string path, std::string relative_path = "");

    public:
        Module* parse_module();
        Import* parse_import();

        AstNode* parse_definitions();

        AstNode* parse_user_type();

        AstNode* parse_variable();
        AstNode* parse_field();

        AstNode* parse_variable_definition();

        Function* parse_function();
        Variable* parse_parameter();

        Statement* parse_statement();
        WhileStatement* parse_while_statement();
        Statement* parse_for_statement();

        IfStatement* parse_if_statement();
        ElifStatement* parse_elif_statement();
        ElseStatement *parse_else_statement();

        ReturnStatement* parse_return_statement();

        AstNode* parse_switch_statement();
        AstNode* parse_switch_cases();

        Statements* parse_statements();

        Type* parse_type();
        Type* parse_function_type();
        Type* parse_tuple_or_function_type();
        Type* parse_primary_type();
        Type* parse_named_type();

        Expression* parse_expression();
        Expression* parse_assignment_expression();
        Expression* parse_cast_expression();

        Expression* parse_logical_or_expression();
        Expression* parse_logical_and_expression();

        Expression* parse_equality_expression();
        Expression* parse_relational_expression();

        Expression* parse_range_expression();

        Expression* parse_arith_expression();
        Expression* parse_term_expression();
        Expression* parse_power_expression();

        Expression* parse_bitwise_or_expression();
        Expression* parse_bitwise_xor_expression();
        Expression* parse_bitwise_and_expression();

        Expression* parse_shift_expression();

        Expression* parse_unary_expression();
        Expression* parse_logical_not();
        Expression* parse_address_of();
        Expression* parse_dereference();
        Expression* parse_bitwise_not();
        Expression* parse_unary_minus();
        Expression* parse_unary_plus();
        Expression* parse_pre_increment();
        Expression* parse_pre_decrement();
        AstNode* parse_sizeof();

        AstNode* parse_postfix_expression();
        Call* parse_call_expression(Expression* obj);
        void parse_call_arguments(Call* call);

        Expression* parse_primary_expression();

        New* parse_new_expression();
        void parse_new_arguments(New* obj);

        Expression* parse_delete_expression();
        Expression* parse_parenthesis_or_tuple_or_sequence();
        Expression* parse_list_expression();
        Expression* parse_array_or_hash_expression();
        Expression* parse_array_expression(Expression* expression);
        Expression *parse_hash_expression(Expression *key);

        AstNode* parse_lambda();

        Expression* parse_generic_application();
        Expression* parse_scope();
        Identifier* parse_identifier();

        Generics* parse_generics();

    private:
        void advance();
        bool lookahead(int kind, int offset=0);
        bool lookahead_same_line(int kind, int offset=0);
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
