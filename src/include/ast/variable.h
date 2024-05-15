#ifndef HAARD_VARIABLE_H
#define HAARD_VARIABLE_H

#include "ast/declaration.h"
#include "ast/types/type.h"
#include "ast/expression.h"

namespace haard {
    class Variable : public Declaration {
    public:
        Variable();
        ~Variable();

    public:
        Type* get_type() const;
        void set_type(Type* newType);

        Expression* get_expression() const;
        void set_expression(Expression* newExpression);

    private:
        Type* type;
        Expression* expression;
    };
}

#endif
