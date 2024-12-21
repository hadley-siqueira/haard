#ifndef HAARD_AST_MODULE_H
#define HAARD_AST_MODULE_H

#include <vector>

#include "haard/ast/ast_node.h"
#include "haard/ast/import.h"
#include "haard/ast/function.h"

namespace haard {
    class Module : public AstNode {
    public:
        Module();
        ~Module();

    public:
        void add_import(Import* import);
        void add_function(Function* function);

        const std::string& get_path() const;
        void set_path(const std::string& path);

    private:
        std::string path;
        std::vector<Import*> imports;
        std::vector<Function*> functions;
        //std::vector<Ast*> children;
    };
}

#endif
