#ifndef HAARD_AST_IDENTIFIER_H
#define HAARD_AST_IDENTIFIER_H

#include "haard/ast/expressions/expression.h"

namespace haard {
    class Identifier : public Expression {
    public:
        Identifier();
        Identifier(Token& token);
        ~Identifier();

    public:
        const Token& get_token() const;
        void set_token(const Token& token);

    private:
        Token token;
    };
}

#endif
