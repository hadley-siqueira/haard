#include "scope/symbol.h"
#include "ast/named_type_descriptor.h"

using namespace haard;

Symbol::Symbol() {

}

Symbol::Symbol(SymbolKind kind) {
    set_kind(kind);
}

SymbolKind Symbol::get_kind() const {
    return kind;
}

void Symbol::set_kind(SymbolKind newKind) {
    kind = newKind;
}

void Symbol::add_descriptor(void* descriptor) {
    descriptors.push_back(descriptor);
}

int Symbol::descriptors_count() {
    return descriptors.size();
}

void* Symbol::get_descriptor(int idx) {
    if (idx < descriptors_count()) {
        return descriptors[idx];
    }

    return nullptr;
}

Type* Symbol::get_type(int idx) {
    Variable* var = (Variable*) descriptors[idx];
    NamedTypeDescriptor* named = (NamedTypeDescriptor*) descriptors[idx];
    Function* func = (Function*) descriptors[idx];

    switch (kind) {
    case SYM_CLASS:
    case SYM_STRUCT:
    case SYM_ENUM:
    case SYM_UNION:
        return named->get_self_type();

    case SYM_FUNCTION:
    case SYM_METHOD:
        return func->get_self_type();

    case SYM_PARAMETER:
    case SYM_LOCAL_VARIABLE:
    case SYM_INSTANCE_VARIABLE:
        return var->get_type();
    }

    return nullptr;
}

bool Symbol::is_function() {
    return kind == SYM_FUNCTION;
}
