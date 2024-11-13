#include <sstream>

#include "haard/ast/types/primitive_type.h"

using namespace haard;

PrimitiveType::PrimitiveType() {

}

PrimitiveType::PrimitiveType(AstKind kind) {
    set_kind(kind);
}

PrimitiveType::PrimitiveType(AstKind kind, Token& token) {
    set_kind(kind);
    set_token(token);
}

const Token& PrimitiveType::get_token() const {
    return token;
}

void PrimitiveType::set_token(const Token& token) {
    this->token = token;
}
