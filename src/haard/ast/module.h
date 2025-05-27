#ifndef HAARD_AST_MODULE_H
#define HAARD_AST_MODULE_H

#include <vector>

#include <haard/ast/ast.h>
#include <haard/ast/import.h>

namespace haard {
    class Module : public Ast {
    public:
        Module();
        ~Module();

    public:
        void add_import(Import* node);

    private:
        std::vector<Import*> imports;
    };
}

#endif
