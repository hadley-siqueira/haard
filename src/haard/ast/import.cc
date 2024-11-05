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

std::string Import::to_json() {
    int i;
    std::stringstream ss;

    ss << "{\"kind\": \"import\"";

    if (alias.get_value() != "") {
        ss << ", \"alias\": " << alias.to_json();
    }

    if (path.size() > 0) {
        ss << ", \"path\": [";

        for (i = 0; i < path.size() - 1; ++i) {
            ss << path[i].to_json() << ", ";
        }

        ss << path[i].to_json();
        ss << "]";
    }

    ss << "}";
    return ss.str();
}

const std::vector<Token>& Import::get_path() const {
    return path;
}

void Import::set_path(const std::vector<Token>& path) {
    this->path = path;
}
