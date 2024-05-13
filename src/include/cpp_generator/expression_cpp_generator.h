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

        void build_call(Call* expr);

        void build_identifier(Identifier* expr);
        void build_boolean_literal(BooleanLiteral* expr);
        void build_integer_literal(IntegerLiteral* expr);

        void build_binop(BinaryOperator* expr, const char* oper);

        std::string get_output();

    private:
        std::stringstream output;
    };
}

#endif
