#include <sstream>

#include "haard/ast/import.h"

using namespace haard;

Import::Import() {
    set_kind(AST_IMPORT);
}

void Import::add_to_path(Token& token) {
    path.push_back(token);
}

bool Import::has_alias() {
    return alias.get_value() != "";
}

const Token &Import::get_alias() const {
    return alias;
}

void Import::set_alias(const Token &alias) {
    this->alias = alias;
}

const std::vector<Token>& Import::get_path() const {
    return path;
}

void Import::set_path(const std::vector<Token>& path) {
    this->path = path;
}
