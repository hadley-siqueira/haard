#ifndef HAARD_AST_BINOP_H
#define HAARD_AST_BINOP_H

#include "token/token.h"
#include "ast/expression.h"

namespace haard {
    class BinOp : public Expression {
        public:
            BinOp();
            BinOp(int kind, Expression* left=nullptr, Expression* right=nullptr);
            BinOp(int kind, Token& token, Expression* left=nullptr, Expression* right=nullptr);
            virtual ~BinOp();

        public:
            Expression* get_left();
            Expression* get_right();

            void set_left(Expression* left);
            void set_right(Expression* right);

        private:
            Expression* left;
            Expression* right;
            int line;
            int column;
    };
}

#endif
