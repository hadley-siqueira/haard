#ifndef HAARD_MODULE_H
#define HAARD_MODULE_H

#include <vector>
#include "ast/ast_node.h"
#include "ast/declaration.h"
#include "ast/import.h"
#include "ast/function.h"
#include "ast/class.h"
#include "ast/struct.h"
#include "ast/union.h"
#include "symbol_table/symbol_table.h"

namespace haard {
    class Module : public AstNode {
    public:
        Module();
        ~Module();

    public:
        void add_import(Import* import);
        int imports_count();
        Import* get_import(int idx);

        void add_function(Function* function);
        void add_class(Class* klass);
        void add_struct(Struct* s);
        void add_union(Union* u);

        int functions_count();
        int classes_count();
        int structs_count();
        int unions_count();

        Class* get_class(int idx);
        Function* get_function(int idx);

        int declarations_count();
        Declaration* get_declaration(int idx);

        Scope* get_scope() const;
        void set_scope(Scope* newScope);

    private:
        std::vector<Import*> imports;
        std::vector<Function*> functions;
        std::vector<Declaration*> declarations;
        std::vector<Class*> classes;
        std::vector<Struct*> structs;
        std::vector<Union*> unions;
        Scope* scope;
    };
}

#endif
