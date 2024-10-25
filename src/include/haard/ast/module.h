#ifndef HAARD_AST_MODULE_H
#define HAARD_AST_MOULE_H

#include <vector>

#include "haard/ast/ast.h"
#include "haard/ast/import.h"

namespace haard {
    class Module : public Ast {
    private:
        std::vector<Import*> imports;
    };
}

#endif
