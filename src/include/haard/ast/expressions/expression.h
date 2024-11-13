#ifndef HAARD_AST_EXPRESSION_H
#define HAARD_AST_EXPRESSION_H

#include "haard/ast/ast.h"
#include "haard/ast/types/type.h"

namespace haard {
    class Expression : public Ast {
    public:
        Expression();
        virtual ~Expression();

    public:
        Type* get_type() const;
        void set_type(Type* type);

    private:
        Type* type;
    };
}

#endif
