#ifndef HAARD_EXPRESSION_H
#define HAARD_EXPRESSION_H

#include "ast/types/type.h"

namespace haard {
    class Expression : public AstNode {
    public:
        Expression();

    public:
        Type* get_type() const;
        void set_type(Type* newType);

    private:
        Type* type;
    };
}

#endif
