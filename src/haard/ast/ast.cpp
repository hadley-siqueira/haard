#include <haard/ast/ast.h>

using namespace haard;

Ast::Ast() {
    make_node(AST_UNKNOWN);
}

u32 Ast::make_node(AstNodeKind kind) {
    AstNode node;

    node.set_kind(kind);
    nodes.push_back(node);

    return nodes.size() - 1;
}

u32 Ast::add_child(u32 parent, u32 child) {
    if (nodes[parent].get_children() == 0) {
        nodes[parent].set_children(child);
        return child;
    }

    auto tmp = nodes[parent].get_children();
    auto last = tmp;

    while (tmp != 0) {
        last = tmp;
        tmp = nodes[tmp].get_sibling();
    }

    nodes[last].set_sibling(child);
    return child;
}

u32 Ast::add_child(u32 parent, u32 last_child, u32 child) {
    if (child == 0) {
        return last_child;
    }

    if (nodes[parent].get_children() == 0) {
        nodes[parent].set_children(child);
    } else {
        nodes[last_child].set_sibling(child);
    }

    return child;
}

void Ast::add_sibling(u32 node, u32 sibling) {
    if (sibling == 0) {
        return;
    }

    nodes[node].set_sibling(sibling);
}
