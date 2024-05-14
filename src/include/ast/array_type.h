#ifndef HAARD_ARRAY_TYPE_H
#define HAARD_ARRAY_TYPE_H

#include "ast/subtyped_type.h"
#include "ast/expression.h"

namespace haard {
    class ArrayType : public SubtypedType {
    public:
        ArrayType(Type* subtype, Expression* expression=nullptr);
        ~ArrayType();

    public:
        Expression* get_expression() const;
        void set_expression(Expression* newExpression);
        std::string to_cpp();

    private:
        Expression* expression;

    };
}

#endif
