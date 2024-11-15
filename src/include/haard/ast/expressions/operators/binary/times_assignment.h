#ifndef HAARD_AST_TIMES_ASSIGNMENT_H
#define HAARD_AST_TIMES_ASSIGNMENT_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class TimesAssignment : public BinaryOperator {
    public:
        TimesAssignment();
        TimesAssignment(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif