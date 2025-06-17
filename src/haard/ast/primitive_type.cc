#include <haard/ast/primitive_type.h>

using namespace haard;

PrimitiveType::PrimitiveType() {
    set_kind(AST_UNKNOWN);
}

PrimitiveType::PrimitiveType(const Token& token) {
    set_token(token);

    switch (token.get_kind()) {
    case TK_U8:
        set_kind(AST_TYPE_U8);
        break;

    case TK_I8:
        set_kind(AST_TYPE_I8);
        break;

    case TK_U16:
        set_kind(AST_TYPE_U16);
        break;

    case TK_I16:
        set_kind(AST_TYPE_I16);
        break;

    case TK_U32:
        set_kind(AST_TYPE_U32);
        break;

    case TK_I32:
        set_kind(AST_TYPE_I32);
        break;

    case TK_U64:
        set_kind(AST_TYPE_U64);
        break;

    case TK_I64:
        set_kind(AST_TYPE_I64);
        break;

    case TK_F32:
        set_kind(AST_TYPE_F32);
        break;

    case TK_F64:
        set_kind(AST_TYPE_F64);
        break;
    }
}

PrimitiveType::~PrimitiveType() {

}

void PrimitiveType::set_token(const Token& token) {
    this->token = token;
}

Token& PrimitiveType::get_token() {
    return token;
}
