#ifndef HAARD_AST_POINTER_TYPE_H
#define HAARD_AST_POINTER_TYPE_H

#include "haard/ast/types/type.h"

namespace haard {
    class PointerType : public Type {
    public:
        PointerType();
        PointerType(Type* subtype, Token& token);
        ~PointerType();

    public:
        const Token& get_token() const;
        void set_token(const Token& token);

        Type* get_subtype() const;
        void set_subtype(Type* subtype);

        bool equals(Type* other);

    private:
        Token token;
        Type* subtype;
    };
}

#endif
