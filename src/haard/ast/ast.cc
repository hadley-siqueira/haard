#include <haard/ast/ast.h>

using namespace haard;

Ast::Ast() {
    set_kind(AST_UNKNOWN);
    set_parent_node(nullptr);
}

Ast::~Ast() {

}

void Ast::set_kind(AstKind kind) {
    this->kind = kind;
}

AstKind Ast::get_kind() {
    return kind;
}

void Ast::set_parent_node(Ast* node) {
    parent_node = node;
}

Ast* Ast::get_parent_node() {
    return parent_node;
}
