#include "semantic/type_linker.h"
#include "ast/types/pointer_type.h"
#include "ast/named_type_descriptor.h"

using namespace haard;

void TypeLinker::build(Type* type) {
    switch (type->get_kind()) {
    case TYPE_POINTER:
        build_pointer_type((PointerType*) type);
        break;

    case TYPE_NAMED:
        build_named_type((NamedType*) type);
        break;
    }
}

void TypeLinker::build_pointer_type(PointerType* type) {
    build(type->get_subtype());
}

void TypeLinker::build_named_type(NamedType* type) {
    Identifier* id = type->get_identifier();
    std::string name = id->get_name().get_value();
    Symbol* sym;

    sym = context->get_scope()->resolve(name);

    if (sym) {
        for (int i = 0; i < sym->descriptors_count(); ++i) {
            SymbolDescriptor* sd = sym->get_descriptor(i);

            if (sd->is_named_type()) {
                NamedTypeDescriptor* nd = (NamedTypeDescriptor*) sd->get_descriptor();

                if (id->get_generics() && nd->get_generics()) {
                    if (id->get_generics()->types_count() == nd->get_generics()->types_count()) {
                        id->set_symbol(sym);
                        id->set_overload_index(i);
                    }
                } else if (id->get_generics() == nullptr && nd->get_generics() == nullptr) {
                    id->set_symbol(sym);
                    id->set_overload_index(i);
                }
            }
        }
    } else {
        logger->error(name + " not in scope");
    }
}

SemanticContext* TypeLinker::get_context() const {
    return context;
}

void TypeLinker::set_context(SemanticContext* newContext) {
    context = newContext;
}
