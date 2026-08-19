#ifndef HAARD_AST_H
#define HAARD_AST_H

#include <vector>
#include <haard/ast/ast_node.h>

namespace haard {
    // Holds the nodes and nothing else. Building them is AstBuilder's job, so
    // that this class stays a plain container: a flat vector of same sized
    // nodes that reference each other by index, which is what makes writing it
    // to a file a matter of dumping bytes.
    class Ast {
        public:
            Ast();

        public:
            u32 push(AstNode& node);

            std::vector<AstNode>& get_nodes();

            // the node every walk starts from, 0 when nothing was built yet.
            // It is set explicitly because a tree is built from the leaves up:
            // the module is the last node to be created, not the first
            void set_root(u32 node);
            u32 get_root();

            AstNode* get_node(u32 index);

        private:
            std::vector<AstNode> nodes;
            u32 root;
    };
}

#endif
