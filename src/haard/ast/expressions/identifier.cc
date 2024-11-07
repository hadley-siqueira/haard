#include "haard/ast/expressions/identifier.h"

using namespace haard;

Identifier::Identifier() {
    set_kind(AST_IDENTIFIER);
}

Identifier::Identifier(Token& token) {
    set_token(token);
}

Identifier::~Identifier() {

}

const Token& Identifier::get_token() const {
    return token;
}

void Identifier::set_token(const Token& token) {
    this->token = token;
}

std::string Identifier::to_json() {
    return "identifier json";
}

std::string Identifier::to_str() {
    return token.get_value();
}
