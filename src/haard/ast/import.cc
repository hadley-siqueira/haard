#include <haard/ast/import.h>

using namespace haard;

Import::Import() {
    set_kind(AST_IMPORT);
}

Import::~Import() {

}

void Import::add_to_path(Token& token) {
    path.push_back(token);
}

void Import::set_alias(Token& token) {
    this->alias = token;
}

void Import::set_token(Token& token) {
    this->token = token;
}
