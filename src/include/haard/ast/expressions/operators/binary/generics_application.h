#ifndef HAARD_AST_GENERICS_APPLICATION_H
#define HAARD_AST_GENERICS_APPLICATION_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"
#include "haard/ast/generics.h"

namespace haard {
    class GenericsApplication : public BinaryOperator {
    public:
        GenericsApplication();
        GenericsApplication(Expression* expression, Generics* generics);

    public:
        Expression* get_expression() const;
        void set_expression(Expression* expression);

        Generics* get_generics() const;
        void set_generics(Generics* generics);
    };
}

#endif
