#ifndef HAARD_AST_PRIMITIVE_TYPE_H
#define HAARD_AST_PRIMITIVE_TYPE_H

#include "haard/ast/types/type.h"

namespace haard {
    class PrimitiveType : public Type {
    public:
        PrimitiveType();
        PrimitiveType(AstKind kind);
        PrimitiveType(AstKind kind, Token& token);

        const Token& get_token() const;
        void set_token(const Token& token);

        bool equals(Type* other);

    private:
        Token token;
    };
}

#endif
