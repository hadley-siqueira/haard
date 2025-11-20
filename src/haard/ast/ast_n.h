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

        private:
            std::vector<Ast*> children;
    };
};

#endif
