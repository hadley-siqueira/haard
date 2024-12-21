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

        void print(AstNode* node);

        void print_module(Module* module);

        void print_import(Import* import);

        void print_user_type(AstNode* node);

        void print_super_type(AstNode* node);

        void print_function(Function* function);
        void print_variable(Variable* var);

        void print_lambda(AstNode* node);
        void print_lambda_return_type(AstNode* node);
        void print_lambda_parameters(AstNode* node);
        void print_lambda_parameter(AstNode* node);
        void print_lambda_parameter_type(AstNode* node);
        void print_lambda_parameter_expression(AstNode* node);
        void print_lambda_statements(AstNode* node);

        /* Statements */
        void print_statements(Statements* stmts);
        void print_while(WhileStatement* stmt);
        void print_for(ForStatement* stmt);
        void print_foreach(ForeachStatement* stmt);
        void print_if(IfStatement* node);
        void print_elif(ElifStatement* node);
        void print_else(ElseStatement* node);
        void print_return(ReturnStatement* node);
        void print_switch(AstNode* node);
        void print_switch_brace(AstNode* node);
        void print_switch_cases(AstNode* node);
        void print_switch_case(AstNode* node);
        void print_switch_default(AstNode* node);
        void print_expression_statement(ExpressionStatement* stmt);

        /* types */
        void print_pointer_type(PointerType* node);
        void print_reference_type(ReferenceType* node);
        void print_list_type(AstNode* node);
        void print_array_type(ArrayType* type);
        void print_tuple_type(TupleType* tuple);
        void print_function_type(FunctionType* ftype);
        void print_named_type(NamedType* type);

        void print_type_list(AstNode *tlist, const char* begin, const char* end);

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

        void print_cast(AstNode* node);

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
        void print_call(Call* node);
        void print_arguments(AstNode* args);
        void print_named_argument(NamedArgument *node);
        void print_pos_increment(AstNode* node);
        void print_pos_decrement(AstNode* node);
        void print_sizeof(AstNode* node);
        void print_delete(Delete* node);
        void print_delete_array(DeleteArray* node);

        void print_parenthesis(Parenthesis* node);
        void print_generic_application(GenericsApplication* node);
        void print_scope(Scope* scope);
        void print_identifier(Identifier* id);
        void print_this(This* node);
        void print_null(Null* node);

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
        void print_index(AstNode* node);
        void print_hash_pair(AstNode* pair);


        void print_logical_not(LogicalNot* un);
        void print_address_of(AddressOf* node);
        void print_dereference(Dereference* node);
        void print_bitwise_not(BitwiseNot* node);
        void print_unary_minus(UnaryMinus* node);
        void print_unary_plus(UnaryPlus* node);
        void print_pre_increment(PreIncrement* node);
        void print_pre_decrement(PreDecrement* node);

//        void print_sizeof_expression(UnaryOperator* un);
//        void print_new_expression(New* expr);
//        void print_delete_expression(Delete* expr);
//        void print_delete_array_expression(DeleteArray* expr);
//        void print_parenthesis_expression(Parenthesis* expr);


//        void print_identifier(Identifier* id);
        void print_generics(const Generics* g);

        void print_tuple(Tuple* node);
        void print_sequence(Sequence* node);
        void print_list(List* node);
        void print_array(AstNode* expr);
        void print_hash(AstNode* expr);


        void print_expression_list(AstNode* list, const char* begin, const char* end, const char* sep=",");

    private:
        void indent();
        void dedent();
        void print_indentation();
        void print_binop(AstNode* node, const char* oper, bool no_space=false);

    private:
        int indent_level;
        std::stringstream out;
    };
}
#endif
