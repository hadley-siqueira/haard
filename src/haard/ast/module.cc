#include <sstream>

#include "haard/ast/module.h"

using namespace haard;

Module::Module() {
    set_kind(AST_MODULE);
}

Module::~Module() {
    for (auto c : children) {
        delete c;
    }
}

void Module::add_import(Import* import) {
    imports.push_back(import);
    children.push_back(import);
}

const std::string &Module::get_path() const {
    return path;
}

void Module::set_path(const std::string &path) {
    this->path = path;
}

std::string Module::to_json() {
    std::stringstream ss;

    ss << "{\"kind\": \"AST_MODULE\"";

    if (imports.size() > 0) {
        ss << ", \"imports\": [";

        bool first = true;
        for (auto i : imports) {
            if (!first) ss << ", ";
            first = false;
            ss << i->to_json();
        }

        ss << "]";
    }

    ss << "}";
    return ss.str();
}
