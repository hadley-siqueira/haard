#ifndef HAARD_AST_ARRAY_TYPE_H
#define HAARD_AST_ARRAY_TYPE_H

#include "haard/ast/types/type.h"

namespace haard {
    class ArrayType : public Type {
    public:
        ArrayType();
        ArrayType(Type* subtype, Token& token);
        ~ArrayType();

        Type* get_subtype() const;
        void set_subtype(Type* type);

        AstNode* get_expression() const;
        void set_expression(AstNode* expression);

        const Token& get_token() const;
        void set_token(const Token& token);

        bool equals(Type* other);

    private:
        Token token;
        Type* subtype;
        AstNode* expression;
    };
}

#endif
