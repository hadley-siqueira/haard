#ifndef HAARD_AST_MODULE_H
#define HAARD_AST_MODULE_H

#include <vector>

#include <haard/ast/ast.h>
#include <haard/ast/import.h>
#include <haard/ast/variable.h>
#include <haard/ast/function.h>

namespace haard {
    class Module : public Ast {
    public:
        Module();
        ~Module();

    public:
        void add_import(Import* node);
        void add_variable(Variable* node);
        void add_function(Function* node);

        std::vector<Import*>& get_imports();
        std::vector<Variable*>& get_variables();

        void set_path(const std::string& path);
        const std::string& get_path();

    private:
        std::string path;
        std::vector<Import*> imports;
        std::vector<Variable*> variables;
        std::vector<Function*> functions;
    };
}

#endif
