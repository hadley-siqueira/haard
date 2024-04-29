#include "ast/import.h"

using namespace haard;

Import::Import() {
    set_kind(AST_IMPORT);
    alias_flag = false;
}

int Import::path_count() {
    return path.size();
}

void Import::add_to_path(Token& token) {
    path.push_back(token);
}

const Token& Import::get_path_token(int idx) {
    return path[idx];
}

bool Import::has_alias() {
    return alias_flag;
}

void Import::set_alias(Token& token) {
    alias = token;
    alias_flag = true;
}

const Token& Import::get_alias() {
    return alias;
}
