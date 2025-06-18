#include <haard/ast/primitive_type.h>

using namespace haard;

PrimitiveType::PrimitiveType() {
    set_kind(AST_UNKNOWN);
}

PrimitiveType::PrimitiveType(const Token& token) {
    set_token(token);
    set_kind(AST_TYPE_PRIMITIVE);
}

PrimitiveType::~PrimitiveType() {

}

void PrimitiveType::set_token(const Token& token) {
    this->token = token;
}

Token& PrimitiveType::get_token() {
    return token;
}
