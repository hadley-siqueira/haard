#ifndef HAARD_AST_NAMED_TYPE_H
#define HAARD_AST_NAMED_TYPE_H

#include <vector>

#include "haard/ast/types/type.h"
#include "haard/ast/generics.h"

namespace haard {
    class NamedType : public Type {
    public:
        NamedType();
        NamedType(Token& alias, Token& name, Generics* generics);
        ~NamedType();

    public:
        const Token& get_alias() const;
        void set_alias(const Token& alias);

        const Token& get_name() const;
        void set_name(const Token& name);

        const Generics* get_generics() const;
        void set_generics(Generics* generics);

        virtual std::string to_json();
        virtual std::string to_str();

    private:
        Token alias;
        Token name;
        Generics* generics;
    };
}

#endif
