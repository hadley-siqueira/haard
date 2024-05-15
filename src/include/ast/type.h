#ifndef HAARD_TYPE_H
#define HAARD_TYPE_H

#include "ast/ast_node.h"
#include "token/token.h"

namespace haard {
    class Type : public AstNode {
    public:
        Type();
        virtual ~Type();

        virtual std::string to_cpp();
        virtual bool equals(Type* other);

    public:
        Type(int kind, const Token& token);

    };
}

#endif
