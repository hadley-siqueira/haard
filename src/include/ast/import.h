#ifndef HAARD_IMPORT_H
#define HAARD_IMPORT_H

#include <vector>
#include "token/token.h"
#include "ast/declaration.h"

namespace haard {
    class Module;

    class Import : public Declaration {
    public:
        Import();

    public:
        int path_count();
        void add_to_path(Token& token);
        const Token& get_path_token(int idx);

        bool has_alias();
        void set_alias(Token& token);
        const Token& get_alias();

        Module* get_module() const;
        void set_module(Module *newModule);

    private:
        std::vector<Token> path;
        Token alias;
        bool alias_flag;
        Module* module;
    };
}

#endif
