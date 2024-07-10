#ifndef HAARD_AST_NODE_H
#define HAARD_AST_NODE_H

#include "haard/token/token.h"

namespace haard {
    class AstNode {
    public:
        virtual ~AstNode();

    public:
        int get_kind() const;
        void set_kind(int newKind);

        const Token& get_token() const;
        void set_token(const Token& newToken);

    private:
        int kind;
        Token token;
    };
}

#endif
