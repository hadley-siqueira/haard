#ifndef HAARD_AST_INDIRECTION_TYPE_H
#define HAARD_AST_INDIRECTION_TYPE_H

#include <haard/ast/type.h>
#include <haard/token/token.h>

namespace haard {
    class IndirectionType : public Type {
    public:
        IndirectionType();
        IndirectionType(Type* type, const Token& token);
        ~IndirectionType();

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
