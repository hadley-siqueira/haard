#ifndef HAARD_AST_THIS_H
#define HAARD_AST_THIS_H

#include "haard/ast/expressions/expression.h"

namespace haard {
    class This : public Expression {
    public:
        This();
        This(Token& token);

    public:
        const Token& get_token() const;
        void set_token(const Token& token);

    private:
        Token token;
    };
}

#endif
