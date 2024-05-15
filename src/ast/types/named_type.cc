#include "ast/types/named_type.h"

using namespace haard;

NamedType::NamedType(Identifier* id) {
    set_kind(TYPE_NAMED);
    set_identifier(id);
}

NamedType::~NamedType() {
    delete identifier;
}

Identifier* NamedType::get_identifier() const {
    return identifier;
}

void NamedType::set_identifier(Identifier* newIdentifier) {
    identifier = newIdentifier;
}

NamedTypeDescriptor* NamedType::get_descriptor() {
    return (NamedTypeDescriptor*) identifier->get_symbol()->get_descriptor();
}

std::string NamedType::to_cpp() {
    return identifier->get_name().get_value();
}

bool NamedType::equals(Type* other) {
    NamedType* other_named = (NamedType*) other;

    if (get_kind() != other->get_kind()) {
        return false;
    }

    return get_descriptor() == other_named->get_descriptor();
}
