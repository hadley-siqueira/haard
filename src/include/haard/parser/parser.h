#ifndef HAARD_PARSER_H
#define HAARD_PARSER_H

#include <string>
#include <vector>
#include <stack>

#include "haard/ast/ast.h"
#include "haard/ast/module.h"
#include "haard/ast/import.h"
#include "haard/ast/function.h"
#include "haard/ast/variable.h"

#include "haard/ast/types/type.h"


#include "haard/ast/statements/statement.h"
#include "haard/ast/statements/statements.h"
#include "haard/ast/statements/while_statement.h"
#include "haard/ast/statements/return_statement.h"

#include "haard/ast/expressions/expression.h"



#include "haard/ast/expressions/identifier.h"



#include "haard/ast/generics.h"

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
        Variable* parse_parameter();

        Statement* parse_statement();
        WhileStatement* parse_while_statement();
        Ast* parse_for_statement();

        Ast* parse_if_statement();
        Ast* parse_elif_statement();
        Ast* parse_else_statement();

        ReturnStatement* parse_return_statement();

        Ast* parse_switch_statement();
        Ast* parse_switch_cases();

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
        Ast* parse_sizeof();
        Ast* parse_simple_unary_operator(AstKind ast_type, TokenKind token_type, const char* oper);

        Ast* parse_postfix_expression();

        Ast* parse_primary_expression();

        Ast* parse_new_expression();
        Ast* parse_delete_expression();
        Ast* parse_parenthesis_or_tuple_or_sequence();
        Ast* parse_list_expression();
        Ast* parse_array_or_hash_expression();
        Ast* parse_hash(Ast* key);

        Ast* parse_lambda();

        Ast* parse_argument_list();

        Expression* parse_generic_application();
        Expression* parse_scope();
        Identifier* parse_identifier();

        Generics* parse_generics();

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
