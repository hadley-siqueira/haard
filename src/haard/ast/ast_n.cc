#include <haard/ast/ast_n.h>

using namespace haard;

AstN::AstN() {

}

AstN::AstN(AstKind kind) {
    set_kind(kind);
}

AstN::~AstN() {
    for (auto child : children) {
        delete child;
    }
}

void AstN::add_child(Ast* child) {
    if (child) {
        children.push_back(child);
    }
}
