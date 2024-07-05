#ifndef HAARD_MODULE_H
#define HAARD_MODULE_H

#include <string>

#include "ast/ast.h"

namespace haard {
    class Module {
    public:

        Ast *getAst() const;
        void setAst(Ast *newAst);

    private:
        std::string path;
        Ast* ast;
    };
}

#endif
