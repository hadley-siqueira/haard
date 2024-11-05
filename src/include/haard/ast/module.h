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

        const std::string& get_path() const;
        void set_path(const std::string& path);

        virtual std::string to_json();

    private:
        std::string path;
        std::vector<Import*> imports;
        //std::vector<Ast*> children;
    };
}

#endif