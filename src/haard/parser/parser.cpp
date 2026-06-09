#include <haard/parser/parser.h>
#include <iostream>

using namespace haard;

Parser::Parser() {
    set_context(nullptr);
    current_token = 0;
    matched = 0;
}

void Parser::set_context(Context* context) {
    this->context = context;
}

void Parser::parse_file(const std::string& path) {
    auto mod = ast->make_node(AST_MODULE);
    u32 last_child = 0;

    while (true) {
        if (lookahead(TK_IMPORT)) {
            last_child = ast->add_child(mod, last_child, parse_import());
        } else {
            break;
        }
    }
}

u32 Parser::parse_import() {
    if (!match(TK_IMPORT)) {
        return 0;
    }

    auto path = parse_import_path();

    if (path == 0) {
        return 0;
    }

    auto alias = parse_import_alias();

    if (alias != 0) {
        
    }

    auto node = ast->make_node(AST_IMPORT);
    ast->add_child(node, path);
    ast->add_sibling(path, alias);

    return node;
}

u32 Parser::parse_import_path() {
    if (!lookahead(TK_IDENTIFIER)) {
        return 0;
    }

    auto node = ast->make_node(AST_IMPORT_PATH);
    match(TK_IDENTIFIER);

    while (match(TK_DOT)) {
        if (match(TK_IDENTIFIER)) {
            ast->add_child(node, last, matched)
        } else {

        }
    }

    if (path == 0) {
        std::cout << "error: missing import path";
        return 0;
    }
}

u32 Parser::parse_identifier() {
    u32 node = 0;

    if (match(TK_IDENTIFIER)) {
        node = ast->make_node(AST_IDENTIFIER);
        node->set_kind(AST_IDENTIFIER);
        node->set_token(matched);
    }

    return node;
}

bool Parser::match(TokenKind kind) {
    if (lookahead(kind)) {
        matched = current_token;
        advance();
        return true;
    }

    return false;
}

bool Parser::lookahead(TokenKind kind) {
    return tokens->get_token(current_token).get_kind() == kind;
}

void Parser::advance() {
    if (current_token < tokens->size()) {
        ++current_token;
    }
}
