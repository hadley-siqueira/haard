#ifndef HAARD_ASTN_H
#define HAARD_ASTN_H

#include <vector>
#include <haard/ast/ast.h>

namespace haard {
    class AstN : public Ast {
        public:
            AstN();
            AstN(AstKind kind);
            ~AstN();

        public:
            void add_child(Ast* child);
            void add_child_as(AstKind kind, Ast* child);
            std::vector<Ast*> get_children();
            std::vector<Ast*> get_children(AstKind kind);
            Ast* get_child(AstKind kind);

            std::string to_json();

        private:
            std::vector<Ast*> children;
    };
};

#endif
