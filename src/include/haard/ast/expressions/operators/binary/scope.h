#ifndef HAARD_AST_SCOPE_H
#define HAARD_AST_SCOPE_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"
#include "haard/ast/expressions/identifier.h"

namespace haard {
    class Scope : public BinaryOperator {
    public:
        Scope();
        Scope(Token& token);

    public:
        virtual std::string to_json();
        virtual std::string to_str();

    public:
        void set_alias(Identifier* alias);
        Identifier* get_alias();

        void set_name(Identifier* name);
        Identifier* get_name();
    };
}

#endif
