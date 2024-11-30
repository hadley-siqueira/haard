#ifndef HAARD_AST_NULL_H
#define HAARD_AST_NULL_H

#include "haard/ast/expressions/expression.h"

namespace haard {
    class Null : public Expression {
    public:
        Null();
        Null(Token& token);

    public:
        const Token& get_token() const;
        void set_token(const Token& token);

    private:
        Token token;
    };
}

#endif
