#ifndef HAARD_AST_H
#define HAARD_AST_H

#include <vector>
#include <haard/ast/ast_node.h>

namespace haard {
    class Ast {
        public:
            Ast();

        public:
            u32 make_node(AstNodeKind kind);
            u32 add_child(u32 parent, u32 child);
            u32 add_child(u32 parent, u32 last_child, u32 child);
            void add_sibling(u32 node, u32 sibling);

        private:
            std::vector<AstNode> nodes;
    };
}

#endif
