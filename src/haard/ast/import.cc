#include "haard/ast/import.h"

using namespace haard;

void Import::add_to_path(Token& value) {
    path.push_back(value);
}

void Import::set_alias(Token& alias) {
    this->alias = alias;
}

bool Import::has_alias() {
    return alias.get_value() != "";
}

const std::vector<Token>& Import::get_path() {
    return path;
}

const Token& Import::get_alias() {
    return alias;
}
