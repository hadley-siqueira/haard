#ifndef HAARD_AST_IMPORT_H
#define HAARD_AST_IMPORT_H

#include <vector>

#include <haard/ast/ast.h>
#include <haard/token/token.h>

namespace haard {
    class Import : public Ast {
    public:
        Import();
        ~Import();

    public:
        void add_to_path(Token& token);
        void set_alias(Token& token);

    private:
        std::vector<Token> path;
        Token alias;
    };
}

#endif
