#include <haard/ast/ast.h>

using namespace haard;

Ast::Ast() {

}

Ast::~Ast() {

}

void Ast::set_kind(AstKind kind) {
    this->kind = kind;
}

AstKind Ast::get_kind() {
    return kind;
}

