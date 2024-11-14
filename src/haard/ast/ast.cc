#include <sstream>

#include "haard/ast/ast.h"
#include "haard/pretty_printer/pretty_printer.h"

using namespace haard;

Ast::Ast() {
    set_kind(AST_UNKNOWN);
}

Ast::Ast(AstKind type) {
    set_kind(type);
}

Ast::Ast(AstKind type, Token& token) {
    set_kind(type);
    set_from_token(token);
}

Ast::~Ast() {
    for (auto child : children) {
        delete child;
    }
}

AstKind Ast::get_kind() const {
    return kind;
}

int Ast::get_line() const {
    return line;
}

int Ast::get_column() const {
    return column;
}

const std::string& Ast::get_value() const {
    return value;
}

void Ast::set_kind(AstKind kind) {
    this->kind = kind;
}

void Ast::set_line(int line) {
    this->line = line;
}

void Ast::set_column(int column) {
    this->column = column;
}

void Ast::set_value(const std::string& value) {
    this->value = value;
}

void Ast::add_child(Ast* child) {
    if (child != nullptr) {
        children.push_back(child);
        child->set_parent(this);
    }
}

void Ast::add_child(AstKind kind, Token& token) {
    Ast* child = new Ast(kind, token);
    add_child(child);
}

void Ast::add_child(AstKind kind, Ast* subchild) {
    Ast* child = new Ast(kind);
    child->add_child(subchild);
    add_child(child);
}

Ast* Ast::get_child(size_t index) {
    if (index < children_count()) {
        return children[index];
    }

    return nullptr;
}

Ast* Ast::get_child(AstKind type) {
    for (auto child : children) {
        if (child->get_kind() == type) {
            return child;
        }
    }

    return nullptr;
}

std::vector<Ast*> Ast::get_children(AstKind kind) {
    std::vector<Ast*> r;

    for (int i = 0; i < children_count(); ++i) {
        if (children[i]->get_kind() == kind) {
            r.push_back(children[i]);
        }
    }

    return r;
}

const std::vector<Ast*>& Ast::get_children() const {
    return children;
}

size_t Ast::children_count() {
    return children.size();
}

void Ast::set_from_token(Token& token) {
    set_line(token.get_line());
    set_column(token.get_column());
    set_value(token.get_value());
}

Ast* Ast::get_parent() const {
    return parent;
}

void Ast::set_parent(Ast* parent) {
    this->parent = parent;
}

Ast* Ast::clone() {
    Ast* tmp = new Ast(get_kind());

    tmp->set_line(get_line());
    tmp->set_column(get_column());
    tmp->set_value(get_value());

    for (auto c : children) {
        tmp->add_child(c->clone());
    }

    return tmp;
}

std::string Ast::to_json() {
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

std::string Ast::to_str() {
    PrettyPrinter p;

    p.print(this);
    return p.get_output();
}
