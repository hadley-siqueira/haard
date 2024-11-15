#ifndef HAARD_AST_MODULO_ASSIGNMENT_H
#define HAARD_AST_MODULO_ASSIGNMENT_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class ModuloAssignment : public BinaryOperator {
    public:
        ModuloAssignment();
        ModuloAssignment(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif