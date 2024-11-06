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
    import->set_parent(this);
}

void Module::add_function(Function* function) {
    functions.push_back(function);
    children.push_back(function);
    function->set_parent(this);
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

std::string Module::to_str() {
    std::stringstream ss;

    for (auto child : children) {
        ss << child->to_str();
        ss << "\n";

        switch (child->get_kind()) {
        case AST_FUNCTION:
        case AST_CLASS:
        case AST_STRUCT:
        case AST_ENUM:
        case AST_UNION:
            ss << "\n";
            break;

        default:
            break;
        }
    }

    return ss.str();
}
