#ifndef HAARD_AST_THIS_EXPRESSION_H
#define HAARD_AST_THIS_EXPRESSION_H

#include "expression.h"

namespace haard {
    class This : public Expression {
    public:
        This();
        This(Token& token);
    };
}

#endif
