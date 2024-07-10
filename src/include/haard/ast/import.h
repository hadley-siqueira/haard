#ifndef HAARD_AST_IMPORT_H
#define HAARD_AST_IMPORT_H

#include <vector>
#include <string>
#include "haard/ast/ast.h"

namespace haard::ast {
    class Import : public Ast {
    public:
        Import();

    public:
        int path_count();
        void add_to_path(const std::string &token);
        const std::string& get_path_member(size_t index);

        bool has_alias();
        void set_alias(const std::string& alias);
        const std::string& get_alias() const;

    private:
        std::vector<std::string> path;
        std::string alias;
    };
}

#endif
