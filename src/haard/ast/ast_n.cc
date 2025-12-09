#include <sstream>
#include <haard/ast/ast_n.h>

using namespace haard;

AstN::AstN() {

}

AstN::AstN(AstKind kind) {
    set_kind(kind);
}

AstN::~AstN() {
    for (auto child : children) {
        delete child;
    }
}

void AstN::add_child(Ast* child) {
    if (child) {
        children.push_back(child);
    }
}

void AstN::add_child_as(AstKind kind, Ast* child) {
    if (child) {
        child->set_kind(kind);
        children.push_back(child);
    }
}

std::vector<Ast*> AstN::get_children() {
    return children;
}

std::vector<Ast*> AstN::get_children(AstKind kind) {
    std::vector<Ast*> tmp;

    for (auto child : children) {
        if (child->get_kind() == kind) {
            tmp.push_back(child);
        }
    }

    return tmp;
}

Ast* AstN::get_child(AstKind kind) {
    for (auto child : children) {
        if (child->get_kind() == kind) {
            return child;
        }
    }

    return nullptr;
}

std::string AstN::to_json() {
    std::stringstream ss;

    ss << "{";
    ss << "\"kind\": \"" << get_kind_as_string() << "\", ";
    ss << "\"token\": " << token.to_json();

    if (children.size() > 0) {
        ss << ", \"children\": [";

        bool first = true;

        for (auto child : children) {
            if (!first) {
                ss << ", ";
            }

            ss << child->to_json();
            first = false;
        }

        ss << "]";
    }

    ss << "}";
    return ss.str();
}
