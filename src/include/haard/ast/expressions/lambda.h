#ifndef HAARD_AST_LAMBDA_H
#define HAARD_AST_LAMBDA_H

#include <vector>

#include "haard/ast/expressions/expression.h"
#include "haard/ast/variable.h"
#include "haard/ast/statements/statements.h"

namespace haard {
    class Lambda : public Expression {
    public:
        Lambda();
        ~Lambda();

    public:
        void add_parameter(Variable* var);

        Type* get_return_type() const;
        void set_return_type(Type* return_type);

        std::vector<Variable*>& get_parameters();
        void set_parameters(const std::vector<Variable*>& parameters);

        Statements* get_statements() const;
        void set_statements(Statements* statements);

    private:
        Type* return_type;
        std::vector<Variable*> parameters;
        Statements* statements;
    };
}

#endif
