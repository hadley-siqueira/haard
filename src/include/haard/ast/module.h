#ifndef HAARD_AST_MODULE_H
#define HAARD_AST_MODULE_H

#include <vector>

#include "haard/ast/import.h"

namespace haard::ast {
    class Module {
    public:
        Module();
        ~Module();

    public:
        void add_import(Import* import);
        int imports_count();
        Import* get_import(size_t idx);
        Import* get_import_with_alias(std::string alias);

    private:
        std::vector<Import*> imports;
    };
}

#endif
