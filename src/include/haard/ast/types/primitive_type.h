#ifndef HAARD_AST_TYPES_PRIMITIVE_TYPE_H
#define HAARD_AST_TYPES_PRIMITIVE_TYPE_H

#include "haard/ast/types/type.h"

namespace haard {
    class PrimitiveType : public Type {
    public:
        PrimitiveType();
        PrimitiveType(AstKind kind, Token& token);

    public:

        const Token& get_token() const;
        void set_token(const Token& token);

    private:
        Token token;
    };
}

#endif
