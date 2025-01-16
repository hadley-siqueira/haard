#include "haard/semantic/symbol.h"
#include "haard/ast/ast.h"

using namespace haard;

Symbol::Symbol() {
    set_kind(SYM_UNKNOWN);
    set_descriptor(nullptr);
    set_symbol_table(nullptr);
}

Symbol::Symbol(SymbolKind kind, AstNode* descriptor) {
    set_kind(kind);
    set_descriptor(descriptor);
    set_symbol_table(nullptr);
}

SymbolKind Symbol::get_kind() const {
    return kind;
}

void Symbol::set_kind(SymbolKind kind) {
    this->kind = kind;
}

AstNode* Symbol::get_descriptor() const {
    return descriptor;
}

void Symbol::set_descriptor(AstNode *descriptor) {
    this->descriptor = descriptor;
}

SymbolTable* Symbol::get_symbol_table() const {
    return symbol_table;
}

void Symbol::set_symbol_table(SymbolTable* symbol_table) {
    this->symbol_table = symbol_table;
}

std::string Symbol::get_name() {
    Variable* var;
    Function* func;
    UserType* ut;

    if (descriptor == nullptr) {
        return "";
    }

    switch (kind) {
    case SYM_VARIABLE:
        var = (Variable*) descriptor;
        return var->get_name().get_value();

    case SYM_FUNCTION:
        func = (Function*) descriptor;
        return func->get_name().get_value();

    case SYM_CLASS:
    case SYM_ENUM:
    case SYM_UNION:
    case SYM_STRUCT:
        ut = (UserType*) descriptor;
        return ut->get_name().get_value();

    default:
        return "";

    }
}
