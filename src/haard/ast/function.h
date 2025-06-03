#ifndef HAARD_AST_FUNCTION_H
#define HAARD_AST_FUNCTION_H

#include <vector>

#include <haard/ast/ast.h>
#include <haard/ast/variable.h>
#include <haard/token/token.h>

namespace haard {
    class Function : public Ast {
    public:
        Function();
        ~Function();

    public:
        void add_parameter(Variable* param);

    private:
        Token name;
        std::vector<Variable*> parameters;
    };
}

#endif
