#include <haard/parser/parser.h>
#include <haard/scanner/scanner.h>
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
    haard::Context ctx;
    haard::Scanner sc;
    sc.set_context(&ctx);
    this->set_context(&ctx);
    this->ast = ctx.get_ast();
    this->tokens = ctx.get_tokens();

    sc.get_tokens(path);    
    parse_module();
    ctx.inspect_tokens();
    ctx.inspect_ast();
}

u32 Parser::parse_module() {
    auto mod = ast->make_node(AST_MODULE);
    u32 last_child = 0;

    while (true) {
        if (lookahead(TK_IMPORT)) {
            last_child = ast->add_child(mod, last_child, parse_import());
        } else {
            break;
        }
    }

    return mod;
}

u32 Parser::parse_import() {
    if (!match(TK_IMPORT)) {
        return 0;
    }

    auto node = ast->make_node(AST_IMPORT);
    auto path = parse_import_path();

    if (path == 0) {
        std::cout << "Error: missing import's path. Expected an identifier but got something else";
        return 0;
    }

    auto alias = parse_import_alias();
    ast->add_child(node, path);
    ast->add_sibling(path, alias);

    return node;
}

u32 Parser::parse_import_path() {
    u32 node = 0;
    u32 last = 0;
    u32 member = 0;

    member = parse_import_path_member();

    if (member == 0) {
        return 0;
    }

    node = ast->make_node(AST_IMPORT_PATH);
    last = ast->add_child(node, member);

    while (match(TK_DOT)) {
        member = parse_import_path_member();

        if (member) {
            last = ast->add_child(node, last, member);
        } else {
            std::cout << "Error: expected an identifier after '.' in import's path\n";
            return 0;
        }
    }
    
    return node;
}

u32 Parser::parse_import_path_member() {
    if (!match(TK_IDENTIFIER)) {
        return 0;
    }

    return ast->make_node_with_token(AST_IMPORT_PATH_MEMBER, matched);
}


u32 Parser::parse_import_alias() {
    u32 node = 0;

    if (match(TK_AS)) {
        if (match(TK_IDENTIFIER)) {
            node = ast->make_node_with_token(AST_IMPORT_ALIAS, matched);
        } else {
            std::cout << "Error: expected an identifier after 'as' in import's alias\n";
        }
    }

    return node;
}

u32 Parser::parse_identifier() {
    u32 node = 0;

    if (match(TK_IDENTIFIER)) {
        node = ast->make_node_with_token(AST_IDENTIFIER, matched);
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
