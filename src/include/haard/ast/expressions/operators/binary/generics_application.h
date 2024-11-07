#ifndef HAARD_AST_GENERICS_APPLICATION_H
#define HAARD_AST_GENERICS_APPLICATION_H

#include "haard/ast/expressions/expression.h"
#include "haard/ast/generics.h"

namespace haard {
    class GenericsApplication : public Expression {
    public:
        GenericsApplication();
        GenericsApplication(Expression* expression, Generics* generics);
        ~GenericsApplication();

    public:
        Expression* get_expression() const;
        void set_expression(Expression* expression);

        Generics* get_generics() const;
        void set_generics(Generics* generics);

        virtual std::string to_json();
        virtual std::string to_str();

    private:
        Expression* expression;
        Generics* generics;
    };
}

#endif
