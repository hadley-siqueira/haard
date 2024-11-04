#include "haard/symbol/symbol.h"

using namespace haard;

Symbol::Symbol() {

}

Symbol::Symbol(SymbolKind kind, const std::string& name, Ast* node) {
    set_kind(kind);
    set_name(name);
    set_ast(node);
}

Ast* Symbol::get_ast() const {
    return ast;
}

void Symbol::set_ast(Ast* ast) {
    this->ast = ast;
}

const std::string& Symbol::get_name() const {
    return name;
}

void Symbol::set_name(const std::string& name) {
    this->name = name;
}

SymbolKind Symbol::get_kind() const {
    return kind;
}

void Symbol::set_kind(SymbolKind kind) {
    this->kind = kind;
}
