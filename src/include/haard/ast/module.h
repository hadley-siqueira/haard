#ifndef HAARD_AST_MODULE_H
#define HAARD_AST_MODULE_H

#include <vector>

#include "haard/ast/ast.h"
#include "haard/ast/import.h"

namespace haard {
    class Module : public Ast {
    public:
        Module();
        ~Module();

    public:
        void add_import(Import* import);
        const std::vector<Import*>& get_imports();

        const std::vector<Ast*>& get_defs();

    private:
        std::vector<Import*> imports;
        std::vector<Ast*> defs;
    };
}

#endif
