#ifndef HAARD_AST_BOXED_TYPE_H
#define HAARD_AST_BOXED_TYPE_H

#include <haard/ast/type.h>
#include <haard/token/token.h>

namespace haard {
    class BoxedType : public Type {
    public:
        BoxedType();
        BoxedType(Type* type, const Token& token);
        ~BoxedType();

    public:
        void set_subtype(Type* type);
        Type* get_subtype();

        void set_token(const Token& token);
        Token& get_token();

    private:
        Token token;
        Type* subtype;
    };
}

#endif
