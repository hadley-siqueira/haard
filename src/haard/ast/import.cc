#include "haard/ast/import.h"

using namespace haard;
using namespace haard::ast;

Import::Import() {
    set_kind(AST_IMPORT);
}

int Import::path_count() {
    return path.size();
}

void Import::add_to_path(const std::string& token) {
    path.push_back(token);
}

const std::string &Import::get_path_member(size_t index) {
    return path[index];
}

bool Import::has_alias() {
    return alias.size() > 0;
}

void Import::set_alias(const std::string& alias) {
    this->alias = alias;
}

const std::string& Import::get_alias() const {
    return alias;
}
