#ifndef HAARD_AST_PRIMITIVE_TYPE_H
#define HAARD_AST_PRIMITIVE_TYPE_H

#include <haard/ast/type.h>
#include <haard/token/token.h>

namespace haard {
    class PrimitiveType : public Type {
    public:
        PrimitiveType();
        PrimitiveType(const Token& token);
        ~PrimitiveType();

    public:
        void set_token(const Token& token);

    private:
        Token token;
    };
}

#endif
