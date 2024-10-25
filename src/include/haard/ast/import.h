#ifndef HAARD_AST_IMPORT_H
#define HAARD_AST_IMPORT_H

#include <string>
#include <vector>

#include "haard/token/token.h"
#include "haard/ast/ast.h"

namespace haard {
    class Import : public Ast {
    public:
        void add_to_path(Token &value);
        void set_alias(Token& alias);

        bool has_alias();

        const std::vector<Token>& get_path();
        const Token& get_alias();

    private:
        std::vector<Token> path;
        Token alias;
    };
}

#endif
