#ifndef HAARD_AST_FUNCTION_H
#define HAARD_AST_FUNCTION_H

#include <vector>

#include "haard/ast/ast.h"
#include "haard/ast/variable.h"

namespace haard {
    class Module;
    class Function : public Ast {
    public:
        Function();
        ~Function();

    public:
        void add_parameter(Variable* parameter);
        void add_variable(Variable* var);

        Type* get_return_type() const;
        void set_return_type(Type* type);

        Module *get_module() const;
        void set_module(Module* module);

        const Token& get_name() const;
        void set_name(const Token& name);

    private:
        Token name;
        Type* return_type;
        Module* module;
        std::vector<Variable*> parameters;
        std::vector<Variable*> variables;
    };
}

#endif
