#ifndef HAARD_AST_REFERENCE_TYPE_H
#define HAARD_AST_REFERENCE_TYPE_H

#include "haard/ast/types/type.h"

namespace haard {
    class ReferenceType : public Type {
    public:
        ReferenceType();
        ReferenceType(Type* subtype, Token& token);
        ~ReferenceType();

    public:
        const Token& get_token() const;
        void set_token(const Token& token);

        Type* get_subtype() const;
        void set_subtype(Type* subtype);

        virtual std::string to_json();
        virtual std::string to_str();

    private:
        Token token;
        Type* subtype;
    };
}

#endif
