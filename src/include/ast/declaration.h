#ifndef HAARD_DECLARATION_H
#define HAARD_DECLARATION_H

#include "ast/ast_node.h"
#include "token/token.h"

namespace haard {
    class Module;

    class Declaration : public AstNode {
    public:
        Declaration();

    public:
        const Token& get_name() const;
        void set_name(const Token& newName);

        Module* get_module() const;
        void set_module(Module *newModule);

        const unsigned get_uid() const;
        void set_uid(const unsigned newUid);

    private:
        unsigned uid;
        Token name;
        Module* module;
    };
}

#endif
