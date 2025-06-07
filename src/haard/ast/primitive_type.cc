#include <haard/ast/primitive_type.h>

using namespace haard;

PrimitiveType::PrimitiveType() {
    set_kind(AST_PRIMITIVE_TYPE);
}

PrimitiveType::PrimitiveType(const Token& token) {
    set_kind(AST_PRIMITIVE_TYPE);
    set_token(token);
}

PrimitiveType::~PrimitiveType() {

}

void PrimitiveType::set_token(const Token& token) {
    this->token = token;
}
