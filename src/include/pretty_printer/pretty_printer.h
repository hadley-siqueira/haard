#ifndef HAARD_PRETTY_PRINTER_H
#define HAARD_PRETTY_PRINTER_H

#include <sstream>

#include "haard/ast/ast.h"

namespace haard {
    class PrettyPrinter {
    public:
        PrettyPrinter();

    public:
        std::string get_output();

        void print(Ast* node);

        void print_module(Ast* module);

        void print_import(Ast* import);
        void print_import_path(Ast* path);
        void print_import_path_member(Ast* member);
        void print_import_alias(Ast* alias);

        void print_class(Ast* node);

        void print_enum(Ast* node);
        void print_enum_super(Ast* node);
        void print_fields(Ast* node);
        void print_functions(Ast* node);
        void print_field(Ast* node);
        void print_field_type(Ast* node);
        void print_field_expresion(Ast* node);

        void print_super_type(Ast* node);
        void print_variables(Ast* node);
        void print_member_variable(Ast* node);
        void print_variable_definition(Ast* node);

        void print_function(Ast* function);
        void print_parameters(Ast* parameters);
        void print_parameter(Ast* parameter);

        void print_lambda(Ast* node);
        void print_lambda_return_type(Ast* node);
        void print_lambda_parameters(Ast* node);
        void print_lambda_parameter(Ast* node);
        void print_lambda_parameter_type(Ast* node);
        void print_lambda_parameter_expression(Ast* node);
        void print_lambda_statements(Ast* node);

        /* Statements */
        void print_statements(Ast* stmts);
        void print_while(Ast* stmt);
        void print_for(Ast* stmt);
        void print_for_init(Ast* node);
        void print_for_update(Ast* node);
        void print_if(Ast* node);
        void print_elif(Ast* node);
        void print_else(Ast* node);
        void print_return(Ast* node);
        void print_switch(Ast* node);
        void print_switch_brace(Ast* node);
        void print_switch_cases(Ast* node);
        void print_switch_case(Ast* node);
        void print_switch_default(Ast* node);
        void print_expression(Ast* stmt, bool has_semicolon=false);

        /* types */
        void print_list_type(Ast* node);

        /* Expressions */
        void print_assignment(Ast* node);
        void print_times_assignment(Ast* node);
        void print_division_assignment(Ast* node);
        void print_integer_division_assignment(Ast* node);
        void print_modulo_assignment(Ast* node);
        void print_minus_assignment(Ast* node);
        void print_plus_assignment(Ast* node);
        void print_shift_left_logical_assignment(Ast* node);
        void print_shift_right_arithmetic_assignment(Ast* node);
        void print_shift_right_logical_assignment(Ast* node);
        void print_bitwise_and_assignment(Ast* node);
        void print_bitwise_xor_assignment(Ast* node);
        void print_bitwise_or_assignment(Ast* node);
        void print_bitwise_not_assignment(Ast* node);

        void print_cast(Ast* node);

        void print_logical_or(Ast* node);
        void print_or(Ast* node);

        void print_logical_and(Ast* node);
        void print_and(Ast* node);

        void print_equal(Ast* node);
        void print_not_equal(Ast* node);

        void print_less_than(Ast* node);
        void print_less_than_or_equal(Ast* node);
        void print_greater_than(Ast* node);
        void print_greater_than_or_equal(Ast* node);
        void print_in(Ast* node);
        void print_not_in(Ast* node);

        void print_inclusive_range(Ast* node);
        void print_exclusive_range(Ast* node);

        void print_plus(Ast* node);
        void print_minus(Ast* node);

        void print_times(Ast* node);
        void print_division(Ast* node);
        void print_modulo(Ast* node);
        void print_integer_division(Ast* node);

        void print_power(Ast* node);
        void print_bitwise_or(Ast* node);
        void print_bitwise_xor(Ast* node);
        void print_bitwise_and(Ast* node);

        void print_shift_left_logical(Ast* node);
        void print_shift_right_logical(Ast* node);
        void print_shift_right_arithmetic(Ast* node);

        void print_dot(Ast* node);
        void print_arrow(Ast* node);
        void print_call(Ast* expr);
        void print_arguments(Ast* args);
        void print_argument_name(Ast* node);
        void print_pos_increment(Ast* node);
        void print_pos_decrement(Ast* node);
        void print_sizeof(Ast* node);

        void print_parenthesis(Ast* node);
        void print_generic_application(Ast* node);
        void print_scope(Ast* scope);
        void print_identifier(Ast* id);

//        void print_class(Class* klass);
//        void print_struct(Struct* st);
//        void print_union(Union* u);




//        void print_for_statement(ForStatement* stmt);

//        void print_if_statement(BranchStatement* stmt);
//        void print_elif_statement(BranchStatement* stmt);
//        void print_else_statement(BranchStatement* stmt);

//        void print_return_statement(ReturnStatement* stmt);

//        void print_expression(Expression* expr);
//        void print_cast_expression(Cast* expr);
//        void print_not_in_expression(NotIn* expr);
        void print_index(Ast* node);
        void print_hash_pair(Ast* pair);

//        void print_binary_operator(BinaryOperator* bin, bool no_space=false);

//        void print_unary_operator(UnaryOperator* un, bool last=false);
        void print_logical_not(Ast* un);
        void print_not(Ast* un);
        void print_address_of(Ast* node);
        void print_dereference(Ast* node);
        void print_bitwise_not(Ast* node);
        void print_unary_minus(Ast* node);
        void print_unary_plus(Ast* node);
        void print_pre_increment(Ast* node);
        void print_pre_decrement(Ast* node);

//        void print_sizeof_expression(UnaryOperator* un);
//        void print_new_expression(New* expr);
//        void print_delete_expression(Delete* expr);
//        void print_delete_array_expression(DeleteArray* expr);
//        void print_parenthesis_expression(Parenthesis* expr);

        void print_tuple_type(Ast *tuple);
        void print_function_type(Ast *type);
        void print_named_type(Ast *type);
        void print_array_type(Ast* type);
        void print_type_list(Ast *tlist, const char* begin, const char* end);

//        void print_identifier(Identifier* id);
        void print_generics(Ast* g);

//        void print_char_literal(CharLiteral* ch);
//        void print_string_literal(StringLiteral* str);
        void print_tuple(Ast* expr);
        void print_list(Ast* expr);
        void print_array(Ast* expr);
        void print_hash(Ast* expr);
        void print_sequence(Ast* expr);

        void print_expression_list(Ast* list, const char* begin, const char* end, const char* sep=",");

    private:
        void indent();
        void dedent();
        void print_indentation();
        void print_binop(Ast* node, const char* oper, bool no_space=false);

    private:
        int indent_level;
        std::stringstream out;
    };
}
#endif
