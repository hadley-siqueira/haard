#ifndef HAARD_PRETTY_PRINTER_H
#define HAARD_PRETTY_PRINTER_H

#include <sstream>

#include "haard/ast/ast.h"
#include "haard/ast/module.h"
#include "haard/ast/import.h"
#include "haard/ast/function.h"
#include "haard/ast/variable.h"

#include "haard/ast/types/type.h"
#include "haard/ast/types/primitive_type.h"
#include "haard/ast/types/pointer_type.h"
#include "haard/ast/types/reference_type.h"
#include "haard/ast/types/list_type.h"
#include "haard/ast/types/array_type.h"
#include "haard/ast/types/tuple_type.h"
#include "haard/ast/types/function_type.h"
#include "haard/ast/types/named_type.h"

#include "haard/ast/statements/statement.h"
#include "haard/ast/statements/statements.h"
#include "haard/ast/statements/while_statement.h"
#include "haard/ast/statements/return_statement.h"
#include "haard/ast/statements/expression_statement.h"

#include "haard/ast/expressions/expression.h"

#include "haard/ast/expressions/operators/binary/binary_operator.h"
#include "haard/ast/expressions/operators/binary/assignment.h"
#include "haard/ast/expressions/operators/binary/bitwise_and_assignment.h"
#include "haard/ast/expressions/operators/binary/bitwise_or_assignment.h"
#include "haard/ast/expressions/operators/binary/bitwise_xor_assignment.h"
#include "haard/ast/expressions/operators/binary/bitwise_not_assignment.h"
#include "haard/ast/expressions/operators/binary/division_assignment.h"
#include "haard/ast/expressions/operators/binary/integer_division_assignment.h"
#include "haard/ast/expressions/operators/binary/minus_assignment.h"
#include "haard/ast/expressions/operators/binary/modulo_assignment.h"
#include "haard/ast/expressions/operators/binary/plus_assignment.h"
#include "haard/ast/expressions/operators/binary/times_assignment.h"
#include "haard/ast/expressions/operators/binary/shift_left_logical_assignment.h"
#include "haard/ast/expressions/operators/binary/shift_right_logical_assignment.h"
#include "haard/ast/expressions/operators/binary/shift_right_arithmetic_assignment.h"
#include "haard/ast/expressions/operators/binary/logical_and.h"
#include "haard/ast/expressions/operators/binary/logical_or.h"
#include "haard/ast/expressions/operators/binary/equal.h"
#include "haard/ast/expressions/operators/binary/not_equal.h"
#include "haard/ast/expressions/operators/binary/less_than.h"
#include "haard/ast/expressions/operators/binary/greater_than.h"
#include "haard/ast/expressions/operators/binary/less_than_or_equal.h"
#include "haard/ast/expressions/operators/binary/greater_than_or_equal.h"
#include "haard/ast/expressions/operators/binary/in.h"
#include "haard/ast/expressions/operators/binary/not_in.h"
#include "haard/ast/expressions/operators/binary/inclusive_range.h"
#include "haard/ast/expressions/operators/binary/exclusive_range.h"
#include "haard/ast/expressions/operators/binary/plus.h"
#include "haard/ast/expressions/operators/binary/minus.h"
#include "haard/ast/expressions/operators/binary/times.h"
#include "haard/ast/expressions/operators/binary/division.h"
#include "haard/ast/expressions/operators/binary/modulo.h"
#include "haard/ast/expressions/operators/binary/integer_division.h"
#include "haard/ast/expressions/operators/binary/power.h"
#include "haard/ast/expressions/operators/binary/shift_left_logical.h"
#include "haard/ast/expressions/operators/binary/shift_right_logical.h"
#include "haard/ast/expressions/operators/binary/shift_right_arithmetic.h"
#include "haard/ast/expressions/operators/binary/bitwise_and.h"
#include "haard/ast/expressions/operators/binary/bitwise_or.h"
#include "haard/ast/expressions/operators/binary/bitwise_xor.h"
#include "haard/ast/expressions/operators/binary/index.h"
#include "haard/ast/expressions/operators/binary/dot.h"
#include "haard/ast/expressions/operators/binary/arrow.h"
#include "haard/ast/expressions/operators/binary/generics_application.h"
#include "haard/ast/expressions/operators/binary/scope.h"

#include "haard/ast/expressions/operators/unary/unary_operator.h"
#include "haard/ast/expressions/operators/unary/unary_plus.h"
#include "haard/ast/expressions/operators/unary/unary_minus.h"
#include "haard/ast/expressions/operators/unary/address_of.h"
#include "haard/ast/expressions/operators/unary/dereference.h"
#include "haard/ast/expressions/operators/unary/bitwise_not.h"
#include "haard/ast/expressions/operators/unary/logical_not.h"
#include "haard/ast/expressions/operators/unary/pre_increment.h"
#include "haard/ast/expressions/operators/unary/pre_decrement.h"
#include "haard/ast/expressions/operators/unary/pos_increment.h"
#include "haard/ast/expressions/operators/unary/pos_decrement.h"

#include "haard/ast/expressions/identifier.h"

#include "haard/ast/expressions/literals/literal.h"
#include "haard/ast/expressions/literals/boolean_literal.h"
#include "haard/ast/expressions/literals/char_literal.h"
#include "haard/ast/expressions/literals/integer_literal.h"
#include "haard/ast/expressions/literals/float_literal.h"
#include "haard/ast/expressions/literals/string_literal.h"

#include "haard/ast/generics.h"

namespace haard {
    class PrettyPrinter {
    public:
        PrettyPrinter();

    public:
        std::string get_output();

        void print(Ast* node);

        void print_module(Module* module);

        void print_import(Import* import);

        void print_user_type(Ast* node);

        void print_super_type(Ast* node);

        void print_function(Function* function);
        void print_variable(Variable* var);

        void print_lambda(Ast* node);
        void print_lambda_return_type(Ast* node);
        void print_lambda_parameters(Ast* node);
        void print_lambda_parameter(Ast* node);
        void print_lambda_parameter_type(Ast* node);
        void print_lambda_parameter_expression(Ast* node);
        void print_lambda_statements(Ast* node);

        /* Statements */
        void print_statements(Statements* stmts);
        void print_while(Ast* stmt);
        void print_for(Ast* stmt);
        void print_for_init(Ast* node);
        void print_for_update(Ast* node);
        void print_if(Ast* node);
        void print_elif(Ast* node);
        void print_else(Ast* node);
        void print_return(ReturnStatement* node);
        void print_switch(Ast* node);
        void print_switch_brace(Ast* node);
        void print_switch_cases(Ast* node);
        void print_switch_case(Ast* node);
        void print_switch_default(Ast* node);
        void print_expression_statement(ExpressionStatement* stmt);

        /* types */
        void print_pointer_type(PointerType* node);
        void print_reference_type(ReferenceType* node);
        void print_list_type(Ast* node);
        void print_array_type(ArrayType* type);
        void print_tuple_type(TupleType* tuple);
        void print_function_type(FunctionType* ftype);
        void print_named_type(NamedType* type);

        void print_type_list(Ast *tlist, const char* begin, const char* end);

        /* Expressions */
        void print_assignment(Assignment* node);
        void print_times_assignment(TimesAssignment* node);
        void print_division_assignment(DivisionAssignment* node);
        void print_integer_division_assignment(IntegerDivisionAssignment* node);
        void print_modulo_assignment(ModuloAssignment* node);
        void print_minus_assignment(MinusAssignment* node);
        void print_plus_assignment(PlusAssignment* node);
        void print_shift_left_logical_assignment(ShiftLeftLogicalAssignment* node);
        void print_shift_right_arithmetic_assignment(ShiftRightArithmeticAssignment* node);
        void print_shift_right_logical_assignment(ShiftRightLogicalAssignment* node);
        void print_bitwise_and_assignment(BitwiseAndAssignment* node);
        void print_bitwise_xor_assignment(BitwiseXorAssignment* node);
        void print_bitwise_or_assignment(BitwiseOrAssignment* node);
        void print_bitwise_not_assignment(BitwiseNotAssignment* node);

        void print_cast(Ast* node);

        void print_logical_or(LogicalOr* node);
        void print_logical_and(LogicalAnd* node);

        void print_equal(Equal* node);
        void print_not_equal(NotEqual* node);

        void print_less_than(LessThan* node);
        void print_less_than_or_equal(LessThanOrEqual* node);
        void print_greater_than(GreaterThan* node);
        void print_greater_than_or_equal(GreaterThanOrEqual* node);
        void print_in(In* node);
        void print_not_in(NotIn* node);

        void print_inclusive_range(InclusiveRange* node);
        void print_exclusive_range(ExclusiveRange* node);

        void print_plus(Plus* node);
        void print_minus(Minus* node);

        void print_times(Times* node);
        void print_division(Division* node);
        void print_modulo(Modulo* node);
        void print_integer_division(IntegerDivision* node);

        void print_power(Power* node);

        void print_bitwise_or(BitwiseOr* node);
        void print_bitwise_xor(BitwiseXor* node);
        void print_bitwise_and(BitwiseAnd* node);

        void print_shift_left_logical(ShiftLeftLogical* node);
        void print_shift_right_logical(ShiftRightLogical* node);
        void print_shift_right_arithmetic(ShiftRightArithmetic* node);

        void print_dot(Dot* node);
        void print_arrow(Arrow* node);
        void print_call(Ast* expr);
        void print_arguments(Ast* args);
        void print_argument_name(Ast* node);
        void print_pos_increment(Ast* node);
        void print_pos_decrement(Ast* node);
        void print_sizeof(Ast* node);

        void print_parenthesis(Ast* node);
        void print_generic_application(GenericsApplication* node);
        void print_scope(Scope* scope);
        void print_identifier(Identifier* id);

        void print_boolean_literal(BooleanLiteral* node);
        void print_char_literal(CharLiteral* node);
        void print_integer_literal(IntegerLiteral* node);
        void print_float_literal(FloatLiteral* node);
        void print_string_literal(StringLiteral* node);

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


        void print_logical_not(LogicalNot* un);
        void print_address_of(AddressOf* node);
        void print_dereference(Dereference *node);
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


//        void print_identifier(Identifier* id);
        void print_generics(const Generics* g);

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
