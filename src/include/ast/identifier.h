#ifndef HAARD_IDENTIFIER_H
#define HAARD_IDENTIFIER_H

#include "token/token.h"
#include "ast/expression.h"
#include "ast/type_list.h"

namespace haard {
    class Identifier : public Expression {
    public:
        Identifier();
        Identifier(Token& alias, Token& name, bool alias_flag, bool global_flag, TypeList* generics);
        ~Identifier();

        const Token& get_name() const;
        void set_name(const Token& newName);

        const Token& get_alias() const;
        void set_alias(const Token& newAlias);

        bool has_global_alias();
        bool has_alias();

        TypeList* get_generics() const;
        void set_generics(TypeList* newGenerics);

    private:
        Token alias;
        bool alias_flag;
        bool global_alias_flag;
        TypeList* generics;
    };
}

#endif
