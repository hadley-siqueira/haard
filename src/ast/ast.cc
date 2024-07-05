#include <sstream>

#include "ast/ast.h"

using namespace haard;

Ast::Ast() {
    set_type(AST_UNKNOWN);
    set_value(nullptr);
}

Ast::Ast(AstKind type) {
    set_type(type);
    set_value(nullptr);
}

Ast::Ast(AstKind type, Token& token) {
    set_type(type);
    set_from_token(token);
}

Ast::~Ast() {
    for (auto child : children) {
        delete child;
    }
}

AstKind Ast::get_type() const {
    return type;
}

int Ast::get_line() const {
    return line;
}

int Ast::get_column() const {
    return column;
}

const char* Ast::get_value() const {
    return value;
}

void Ast::set_type(AstKind type) {
    this->type = type;
}

void Ast::set_line(int line) {
    this->line = line;
}

void Ast::set_column(int column) {
    this->column = column;
}

void Ast::set_value(const char* value) {
    this->value = value;
}

void Ast::add_child(Ast* child) {
    if (child != nullptr) {
        children.push_back(child);
    }
}

Ast* Ast::get_child(size_t index) {
    if (index < children_count()) {
        return children[index];
    }

    return nullptr;
}

size_t Ast::children_count() {
    return children.size();
}

void Ast::set_from_token(Token& token) {
    set_line(token.get_line());
    set_column(token.get_column());
    set_value(token.get_value());
}
/*
std::string Ast::to_json() {
    std::stringstream ss;

    switch (type) {
    case AST_ID:
        ss << "{\"type\": \"identifier\", \"value\": " << get_value() << "}";
        break;

    default:
        break;
    }

    return ss.str();
}*/
