#ifndef HAARD_AST_MODULE_H
#define HAARD_AST_MODULE_H

#include <vector>

#include "haard/ast/ast_node.h"
#include "haard/ast/import.h"
#include "haard/ast/function.h"
#include "haard/ast/class.h"
#include "haard/semantic/symbol_table.h"

namespace haard {
    class Module : public AstNode {
    public:
        Module();
        ~Module();

    public:
        void add_import(Import* import);
        void add_function(Function* function);
        void add_class(Class* klass);

        std::string get_path();
        void set_path(const std::string& path);

        SymbolTable* get_symbol_table() const;
        void set_symbol_table(SymbolTable* symbol_table);

        const std::vector<Class*>& get_classes() const;
        void set_classes(const std::vector<Class*>& classes);

        const std::vector<Function *> &get_functions() const;
        void set_functions(const std::vector<Function *> &functions);

    private:
        std::string path;
        std::vector<Import*> imports;
        std::vector<Function*> functions;
        std::vector<Class*> classes;
        SymbolTable* symbol_table;
        //std::vector<Ast*> children;
    };
}

#endif
