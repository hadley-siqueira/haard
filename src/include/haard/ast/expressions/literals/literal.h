#ifndef HAARD_AST_LITERAL_H
#define HAARD_AST_LITERAL_H

#include "haard/ast/expressions/expression.h"

namespace haard {
    class Literal : public Expression {
    public:
        const Token& get_token() const;
        void set_token(const Token& token);

    private:
        Token token;
    };
}

#endif
