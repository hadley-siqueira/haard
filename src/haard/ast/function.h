#ifndef HAARD_AST_FUNCTION_H
#define HAARD_AST_FUNCTION_H

#include <vector>

#include <haard/ast/ast.h>
#include <haard/ast/variable.h>
#include <haard/ast/type.h>
#include <haard/token/token.h>

namespace haard {
    class Function : public Ast {
    public:
        Function();
        ~Function();

    public:
        void add_parameter(Variable* param);

        void set_return_type(Type* type);
        Type* get_return_type();

        void set_name(const Token& token);

    private:
        Token name;
        Type* return_type;
        std::vector<Variable*> parameters;
    };
}

#endif
