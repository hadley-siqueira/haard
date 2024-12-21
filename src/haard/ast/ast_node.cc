#include <sstream>

#include "haard/ast/ast_node.h"
#include "haard/pretty_printer/pretty_printer.h"

using namespace haard;

AstNode::AstNode() {
    set_kind(AST_UNKNOWN);
}

AstNode::AstNode(AstKind type) {
    set_kind(type);
}

AstNode::AstNode(AstKind type, Token& token) {
    set_kind(type);
    set_from_token(token);
}

AstNode::~AstNode() {
    for (auto child : children) {
        delete child;
    }
}

AstKind AstNode::get_kind() const {
    return kind;
}

int AstNode::get_line() const {
    return line;
}

int AstNode::get_column() const {
    return column;
}

const std::string& AstNode::get_value() const {
    return value;
}

void AstNode::set_kind(AstKind kind) {
    this->kind = kind;
}

void AstNode::set_line(int line) {
    this->line = line;
}

void AstNode::set_column(int column) {
    this->column = column;
}

void AstNode::set_value(const std::string& value) {
    this->value = value;
}

void AstNode::add_child(AstNode* child) {
    if (child != nullptr) {
        children.push_back(child);
        child->set_parent(this);
    }
}

void AstNode::add_child(AstKind kind, Token& token) {
    AstNode* child = new AstNode(kind, token);
    add_child(child);
}

void AstNode::add_child(AstKind kind, AstNode* subchild) {
    AstNode* child = new AstNode(kind);
    child->add_child(subchild);
    add_child(child);
}

AstNode* AstNode::get_child(size_t index) {
    if (index < children_count()) {
        return children[index];
    }

    return nullptr;
}

AstNode* AstNode::get_child(AstKind type) {
    for (auto child : children) {
        if (child->get_kind() == type) {
            return child;
        }
    }

    return nullptr;
}

std::vector<AstNode*> AstNode::get_children(AstKind kind) {
    std::vector<AstNode*> r;

    for (int i = 0; i < children_count(); ++i) {
        if (children[i]->get_kind() == kind) {
            r.push_back(children[i]);
        }
    }

    return r;
}

const std::vector<AstNode*>& AstNode::get_children() const {
    return children;
}

size_t AstNode::children_count() {
    return children.size();
}

void AstNode::set_from_token(Token& token) {
    set_line(token.get_line());
    set_column(token.get_column());
    set_value(token.get_value());
}

AstNode* AstNode::get_parent() const {
    return parent;
}

void AstNode::set_parent(AstNode* parent) {
    this->parent = parent;
}

AstNode* AstNode::clone() {
    AstNode* tmp = new AstNode(get_kind());

    tmp->set_line(get_line());
    tmp->set_column(get_column());
    tmp->set_value(get_value());

    for (auto c : children) {
        tmp->add_child(c->clone());
    }

    return tmp;
}

std::string AstNode::to_json() {
    int i;
    std::stringstream ss;

    ss << "{";
    ss << "\"kind\": " << kind;

    if (value != "") {
        ss << ", \"value\": \"" << value << "\"";
    }

    if (children_count() > 0) {
        ss << ", \"children\": [";
        for (i = 0; i < children_count() - 1; ++i) {
            ss << children[i]->to_json() << ", ";
        }

        ss << children[i]->to_json();
        ss << "]";
    }

    ss << "}";
    return ss.str();
}

std::string AstNode::to_str() {
    PrettyPrinter p;

    p.print(this);
    return p.get_output();
}
