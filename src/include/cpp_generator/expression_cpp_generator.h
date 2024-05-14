#ifndef HAARD_EXPRESSION_CPP_GENERATOR_H
#define HAARD_EXPRESSION_CPP_GENERATOR_H

#include <sstream>

#include "ast/ast.h"

namespace haard {
    class ExpressionCppGenerator {
    public:
        void build(Expression* expr);

        void build_assignment(Assignment* expr);

        void build_plus(Plus* expr);
        void build_minus(Minus* expr);

        void build_times(Times* expr);
        void build_division(Division* expr);
        void build_modulo(Modulo* expr);
        void build_interger_division(IntegerDivision* expr);

        void build_bitwise_or(BitwiseOr* expr);
        void build_bitwise_xor(BitwiseXor* expr);
        void build_bitwise_and(BitwiseAnd* expr);
        void build_shift_left_logical(ShiftLeftLogical* expr);
        void build_shift_right_logical(ShiftRightLogical* expr);
        void build_shift_right_arithmetic(ShiftRightArithmetic* expr);

        void build_logical_not(LogicalNot* expr);
        void build_address_of(AddressOf* expr);
        void build_dereference(Dereference* expr);
        void build_bitwise_not(BitwiseNot* expr);
        void build_unary_minus(UnaryMinus* expr);
        void build_unary_plus(UnaryPlus* expr);
        void build_double_dolar(DoubleDolar* expr);
        void build_new(New* expr);
        void build_delete(Delete* expr);
        void build_delete_array(DeleteArray* expr);

        void build_dot(Dot* expr);
        void build_arrow(Arrow* expr);
        void build_index(Index* expr);

        void build_call(Call* expr);

        void build_identifier(Identifier* expr);
        void build_boolean_literal(BooleanLiteral* expr);
        void build_integer_literal(IntegerLiteral* expr);
        void build_float_literal(FloatLiteral* expr);
        void build_double_literal(DoubleLiteral* expr);
        void build_char_literal(CharLiteral* expr);
        void build_string_literal(StringLiteral* expr);
        void build_parenthesis(Parenthesis* expr);

        void build_binop(BinaryOperator* expr, const char* oper, bool no_space=false);
        void build_unary(UnaryOperator* expr, const char* oper, bool last=false);

        std::string get_output();

    private:
        std::stringstream output;
    };
}

#endif
