#ifndef HAARD_AST_LITERAL_H
#define HAARD_AST_LITERAL_H

#include <haard/ast/expression.h>
#include <haard/token/token.h>

namespace haard {
    class Literal : public Expression {
    public:
        Literal();
        Literal(const Token& token);
        ~Literal();

    public:
        void set_token(const Token& token);
        Token& get_token();

    private:
        Token token;
    };
}

#endif
