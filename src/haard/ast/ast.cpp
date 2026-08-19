#include <haard/ast/ast.h>

using namespace haard;

Ast::Ast() {
    AstNode unknown;

    // index 0 is the sentinel every 'no node here' points at
    nodes.push_back(unknown);
    root = 0;
}

u32 Ast::push(AstNode& node) {
    nodes.push_back(node);

    return nodes.size() - 1;
}

std::vector<AstNode>& Ast::get_nodes() {
    return nodes;
}

void Ast::set_root(u32 node) {
    root = node;
}

u32 Ast::get_root() {
    return root;
}

// out of range gives back the unknown sentinel at index 0, the same way
// TokenStream::get_token gives back its eof: a caller that walks a broken tree
// stops instead of reading past the vector
AstNode* Ast::get_node(u32 index) {
    if (index >= nodes.size()) {
        return &nodes[0];
    }

    return &nodes[index];
}
