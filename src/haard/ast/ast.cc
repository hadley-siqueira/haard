#include <sstream>
#include <haard/ast/ast.h>

using namespace haard;

Ast::Ast() {
    set_kind(AST_UNKNOWN);
}

Ast::Ast(AstKind kind) {
    set_kind(kind);
}

Ast::~Ast() {

}

void Ast::set_kind(AstKind kind) {
    this->kind = kind;
}

void Ast::set_token(Token& token) {
    this->token = token;
}

AstKind Ast::get_kind() {
    return kind;
}

Token Ast::get_token() {
    return token;
}

std::string Ast::to_json() {
    std::stringstream ss;

    ss << "{";
    ss << "\"kind\": \"" << get_kind_as_string() << "\", ";
    ss << "\"token\": " << token.to_json();
    ss << "}";

    return ss.str();
}

std::string Ast::get_kind_as_string() {
    switch (kind) {
        case AST_MODULE:
            return "AST_MODULE";

        case AST_IMPORT:
            return "AST_IMPORT";

        case AST_IMPORT_PATH:
            return "AST_IMPORT_PATH";

        case AST_IMPORT_ALIAS:
            return "AST_IMPORT_ALIAS";

        case AST_DEF:
            return "AST_DEF";

        case AST_IDENTIFIER:
            return "AST_IDENTIFIER";

        default:
            return "AST_UNKNOWN";
    }
}
