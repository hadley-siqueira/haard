#ifndef HAARD_PARSER_H
#define HAARD_PARSER_H

#include <string>
#include <vector>
#include <stack>
#include "log/logger.h"
#include "ast/ast.h"
#include "token/token.h"

namespace haard {
    class Parser {
    public:
        Parser();

    public:
        Ast* read(std::string path, std::string relative_path = "");

    public:
        Ast* parse_module();

        Ast* parse_imports();
        Ast* parse_import();
        Ast* parse_import_path();
        Ast* parse_import_path_member();

        Ast* parse_definitions();

        Class* parse_class();
        Struct* parse_struct();
        Union* parse_union();
        Enum* parse_enum();

        Variable* parse_variable();
        Variable* parse_enum_variable();

        Ast* parse_function();
        Ast* parse_parameters();
        Ast* parse_parameter();

        Ast* parse_statement();
        WhileStatement* parse_while_statement();
        ForStatement* parse_for_statement();

        BranchStatement* parse_if_statement();
        BranchStatement* parse_elif_statement();
        BranchStatement* parse_else_statement();

        void parse_for_init_or_range(ForStatement* stmt);
        void parse_for_test(ForStatement* stmt);
        void parse_for_update(ForStatement* stmt);
        Statement* parse_return_statement();
        Ast* parse_statements();

        Ast* parse_type();
        Type* parse_function_type();
        Ast* parse_tuple_or_function_type();
        Ast* parse_primary_type();

        Ast* parse_expression();
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
        Ast* parse_simple_unary_operator(AstType ast_type, TokenKind token_type, const char* oper);

        Ast* parse_postfix_expression();

        Ast* parse_primary_expression();

        Expression* parse_new_expression();
        Expression* parse_delete_expression();
        Ast* parse_parenthesis_or_tuple_or_sequence();
        Ast* parse_list_expression();
        Ast* parse_array_or_hash_expression();
        Ast* parse_hash(Ast* key);

        Ast* parse_argument_list();

        Ast* parse_generic_instantiation();
        Ast* parse_scope();
        Ast* parse_identifier();

        Ast* parse_generics();
        Ast* parse_type_list(AstType kind);

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
        Module* module;
        Token matched;
        int idx;

        std::vector<Token> tokens;
        std::stack<int> indent_stack;
    };
}

#endif
