#include <haard/ast/boxed_type.h>

using namespace haard;

BoxedType::BoxedType() {
    set_kind(AST_UNKNOWN);
    set_subtype(nullptr);
}

BoxedType::BoxedType(Type* type, const Token& token) {
    set_token(token);
    set_subtype(type);

    switch (token.get_kind()) {
    case TK_BITWISE_AND:
        set_kind(AST_TYPE_REFERENCE);
        break;

    case TK_TIMES:
    case TK_POWER:
        set_kind(AST_TYPE_POINTER);
        break;

    case TK_LOGICAL_AND:
        set_kind(AST_TYPE_RVALUE_REFERENCE);
        break;
    }
}

BoxedType::~BoxedType() {
    delete subtype;
}

void BoxedType::set_subtype(Type* type) {
    this->subtype = type;

    if (type != nullptr) {
        type->set_parent_node(this);
    }
}

Type* BoxedType::get_subtype() {
    return subtype;
}

void BoxedType::set_token(const Token& token) {
    this->token = token;
}

Token& BoxedType::get_token() {
    return token;
}
