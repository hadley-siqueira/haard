#include <haard/ast/indirection_type.h>

using namespace haard;

IndirectionType::IndirectionType() {
    set_kind(AST_UNKNOWN);
    set_subtype(nullptr);
}

IndirectionType::IndirectionType(Type* type, const Token& token) {
    set_token(token);
    set_subtype(type);
    set_kind(AST_TYPE_INDIRECTION);
}

IndirectionType::~IndirectionType() {
    delete subtype;
}

void IndirectionType::set_subtype(Type* type) {
    this->subtype = type;

    if (type != nullptr) {
        type->set_parent_node(this);
    }
}

Type* IndirectionType::get_subtype() {
    return subtype;
}

void IndirectionType::set_token(const Token& token) {
    this->token = token;
}

Token& IndirectionType::get_token() {
    return token;
}
