#ifndef HAARD_AST_IMPORT_H
#define HAARD_AST_IMPORT_H

#include <vector>

#include "haard/token/token.h"
#include "haard/ast/ast.h"

namespace haard {
    class Import : public Ast {
    public:
        Import();

    public:
        void add_to_path(Token& token);

        bool has_alias();
        const Token& get_alias() const;
        void set_alias(const Token& alias);

        const std::vector<Token>& get_path() const;
        void set_path(const std::vector<Token>& path);

    private:
        std::vector<Token> path;
        Token alias;
    };
}

#endif
