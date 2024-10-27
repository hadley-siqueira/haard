#ifndef HAARD_AST_VARIABLE_H
#define HAARD_AST_VARIABLE_H

#include "haard/token/token.h"
#include "haard/ast/ast.h"
#include "haard/ast/type.h"

namespace haard {
    class Variable : public Ast {
    public:
        Variable();
        ~Variable();

    public:
        const Token& get_name() const;
        void set_name(const Token& name);

        Type* get_type() const;
        void set_type(Type* type);

    private:
        Token name;
        Type* type;
    };
}

#endif
