#ifndef HAARD_AST_NAMED_TYPE_H
#define HAARD_AST_NAMED_TYPE_H

#include <vector>

#include "haard/ast/types/type.h"

namespace haard {
    class NamedType : public Type {
    public:
        NamedType();
        NamedType(Token& alias, Token& name, std::vector<Type*>& generics);
        ~NamedType();

    public:
        const Token& get_alias() const;
        void set_alias(const Token& alias);

        const Token& get_name() const;
        void set_name(const Token& name);

        const std::vector<Type*>& get_generics() const;
        void set_generics(const std::vector<Type*>& generics);

    private:
        Token alias;
        Token name;
        std::vector<Type*> generics;
    };
}

#endif
