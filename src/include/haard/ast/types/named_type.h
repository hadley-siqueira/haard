#ifndef HAARD_AST_NAMED_TYPE_H
#define HAARD_AST_NAMED_TYPE_H

#include <vector>

#include "haard/ast/types/type.h"
#include "haard/ast/expressions/expression.h"

namespace haard {
    class NamedType : public Type {
    public:
        NamedType();
        NamedType(Expression* expression);
        ~NamedType();

    public:
        Expression* get_name_expression() const;
        void set_name_expression(Expression* expression);

    private:
        Expression* name_expression;
    };
}

#endif
