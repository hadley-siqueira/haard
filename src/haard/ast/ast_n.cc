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
