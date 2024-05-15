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
#include "ast/enum.h"
#include "scope/scope.h"

namespace haard {
    class Module : public AstNode {
    public:
        Module();
        ~Module();

    public:
        void add_import(Import* import);
        int imports_count();
        Import* get_import(int idx);
        Import* get_import_with_alias(std::string alias);

        void add_function(Function* function);
        void add_class(Class* klass);
        void add_struct(Struct* s);
        void add_union(Union* u);
        void add_enum(Enum* e);

        int functions_count();
        int classes_count();
        int structs_count();
        int unions_count();
        int enums_count();

        Class* get_class(int idx);
        Struct* get_struct(int idx);
        Union* get_union(int idx);
        Enum* get_enum(int idx);
        Function* get_function(int idx);

        int declarations_count();
        Declaration* get_declaration(int idx);

        Scope* get_scope() const;
        void set_scope(Scope* newScope);

        std::string get_qualified_path();
        std::string get_cpp_namespace();
        std::string get_cpp_path();

        const std::string& get_path() const;
        void set_path(const std::string& newPath);

        const std::string &get_relative_path() const;
        void set_relative_path(const std::string &newRelative_path);

    private:
        std::string path;
        std::string relative_path;
        std::vector<Import*> imports;
        std::vector<Function*> functions;
        std::vector<Declaration*> declarations;
        std::vector<Class*> classes;
        std::vector<Struct*> structs;
        std::vector<Union*> unions;
        std::vector<Enum*> enums;
        Scope* scope;
    };
}

#endif
