// The parser of the first attempt, kept for reference and NOT built: it is a
// half written refactor that does not compile, and the new parser.cpp is being
// written from scratch beside it. Read it for the grammar it already sketched;
// do not try to fix it.
#include <haard/parser/parser_old.h>
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
    haard::Scanner sc;
    sc.set_context(context);
    this->ast = context->get_ast();
    this->tokens = context->get_tokens();

    sc.get_tokens(path);    
    parse_module();
}

u32 Parser::parse_module() {
    auto mod = ast->make_node(AST_MODULE);
    u32 last_child = 0;

    while (true) {
        if (lookahead(TK_IMPORT)) {
            last_child = ast->add_child(mod, last_child, parse_import());
        } else if (lookahead(TK_LET)) {
            last_child = ast->add_child(mod, last_child, parse_let_declaration());
        } else if (lookahead(TK_CONST)) {
            last_child = ast->add_child(mod, last_child, parse_const_declaration());
        } else {
            break;
        }
    }

    return mod;
}

u32 Parser::parse_import() {
    u32 token = 0;
    u32 path = 0;
    u32 alias = 0;

    auto token = expect_token(TK_IMPORT);
    auto path = parse_import_path();

    if (lookahead(TK_AS)) {
        alias = parse_import_alias();
    }

    return make_import(token, path, alias);;
}

// import foo.
//            ^ Expected an <identifier>

u32 make_import(u32 token, u32 path, u32 alias) {
    u32 node = ast_factory->make_node(AST_IMPORT, token);
    u32 last = 0;

    ast_factory->add_child(node, last, path);
    ast_factory->add_child(node, last, alias);

    return node;
}

u32 Parser::parse_import_path() {
    u32 node = 0;
    u32 last = 0;
    u32 member = 0;

    builder.start();

    member = parse_identifier();

    node = ast->make_node(AST_IMPORT_PATH);
    last = ast->add_child(node, member);

    while (match(TK_DOT)) {
        member = parse_identifier();

        if (member) {
            last = ast->add_child(node, last, member);
        } else {
            std::cout << "Error: expected an identifier after '.' in import's path\n";
            return 0;
        }
    }
    
    builder.finish();
    return builder.make_import_path();
}

u32 Parser::parse_import_path_member() {
    if (!match(TK_IDENTIFIER)) {
        return 0;
    }

    return ast->make_node_with_token(AST_IMPORT_PATH_SEGMENT, matched);
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

u32 Parser::parse_let_declaration() {
    u32 node = 0;

    if (!match(TK_LET)) {
        return 0;
    }

    node = ast->make_node_with_token(AST_LET_DECLARATION, matched);
    auto binding = parse_binding();

    if (binding == 0) {
        return 0;
    }

    ast->add_child(node, binding);
    return node;
}

u32 Parser::parse_const_declaration() {
    u32 node = 0;

    if (!match(TK_CONST)) {
        return 0;
    }

    node = ast->make_node_with_token(AST_CONST_DECLARATION, matched);
    auto binding = parse_binding();

    if (binding == 0) {
        return 0;
    }

    ast->add_child(node, binding);
    return node;
}

u32 Parser::parse_binding() {
    auto node = ast->make_node(AST_BINDING);
    auto name = parse_binding_name();
    auto type = parse_binding_type();
    auto expr = parse_binding_expression();

    if (name == 0) {
        return 0;
    }

    auto last = ast->add_child(node, name);
    last = ast->add_child(node, last, type);
    last = ast->add_child(node, last, expr);

    return node;
}

u32 Parser::parse_binding_type() {
    if (!match(TK_COLON)) {
        return 0;
    }
    
    u32 node = ast->make_node(AST_BINDING_TYPE);
    u32 type = parse_type();

    if (type == 0) {
        return 0;
    }

    ast->add_child(node, type);
    return node;
}

u32 Parser::parse_binding_expression() {
    if (!match(TK_ASSIGNMENT)) {
        return 0;
    }

    u32 node = ast->make_node(AST_BINDING_EXPRESSION);
    u32 expr = parse_expression();

    if (expr == 0) {
        return 0;
    }

    ast->add_child(node, expr);
    return node;
}

u32 Parser::parse_binding_name() {
    return parse_identifier();
}

u32 Parser::parse_type() {
    return parse_identifier();
}

u32 Parser::parse_expression() {
    return parse_arith_expression();
}

u32 Parser::parse_arith_expression() {
    u32 left = 0;
    u32 right = 0;
    u32 last = 0;
    u32 node = parse_primary_expression();

    if (node == 0) {
        return 0;
    }

    while (true) {
        if (match(TK_PLUS)) {
            left = node;
            node = ast->make_node_with_token(AST_PLUS, matched);
            last = ast->add_child(node, left);
            right = parse_primary_expression();

            if (right == 0) {
                std::cout << "error: missing right expr in '+'\n";
                return 0;
            } else {
                ast->add_child(node, last, right);
            }
        } else if (match(TK_MINUS)) {
            left = node;
            node = ast->make_node_with_token(AST_MINUS, matched);
            last = ast->add_child(node, left);
            right = parse_primary_expression();

            if (right == 0) {
                std::cout << "error: missing right expr in '-'\n";
                return 0;
            } else {
                ast->add_child(node, last, right);
            }
        } else {
            break;
        }
    }

    return node;
}

u32 Parser::parse_primary_expression() {
    if (lookahead(TK_IDENTIFIER) || lookahead(TK_SCOPE)) {
        return parse_scoped_identifier();
    }

    return 0;
}

u32 Parser::parse_scoped_identifier() {
    u32 alias;
    u32 oper;
    u32 name;

    name = expect_token(TK_IDENTIFIER);

    if (match(TK_SCOPE)) {
        oper = matched;
        alias = name;
        auto name = expect_token(TK_SCOPE);
    }

    auto id = expect_token(TK_IDENTIFIER);

    return make_binary(oper, alias, id);
}

u32 Parser::parse_identifier_as(AstNodeKind kind) {
    u32 node = parse_identifier();

    if (node != 0) {
        ast->set_node_kind(node, kind);
    }

    return node;
}

u32 Parser::parse_identifier() {
    u32 token;

    token = expect_token(TK_IDENTIFIER);
    return ast_builder.make_identifier(token);
}

u32 make_identifier(u32 token) {
    return ast_factory->make_node(AST_IDENTIFIER, token);
}

u32 Parser::expect_token(TokenKind kind) {
    if (match(kind)) {
        return matched;
    }

    logger.error("expected")
    has_errors = true;


    // /path/to/file.hd:34:12: error: expected %{kind} but got a %{matched.kind}
}

void Parser::synchronize() {
    while (!sync_token()) {
        advance();
    }
}

bool Parser::sync_token() {
    return lookahead(TK_DEF)
        || lookahead(TK_CLASS)
        || lookahead(TK_UNION)
        || lookahead(TK_ENUM)
        || lookahead(TK_STRUCT)
        || lookahead(TK_IF)
        || lookahead(TK_WHILE)
        || lookahead(TK_FOR)
        || lookahead(TK_RETURN)
        || lookahead(TK_CONTINUE)
        || lookahead(TK_YIELD);
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
