#ifndef HAARD_AST_VARIABLE_H
#define HAARD_AST_VARIABLE_H

#include "haard/ast/ast_node.h"
#include "haard/ast/types/type.h"

namespace haard {
    class Variable : public AstNode {
    public:
        Variable();
        ~Variable();

    public:
        const Token& get_name() const;
        void set_name(const Token& name);

        Type* get_type() const;
        void set_type(Type* type);

        AstNode* get_expression() const;
        void set_expression(AstNode* expression);

    private:
        Token name;
        Type* type;
        AstNode* expression;
    };
}

#endif
