#include <iostream>
#include <haard/parser/parser.h>
#include <haard/scanner/scanner.h>
#include <haard/ast/ast_n.h>

using namespace haard;

Ast* Parser::parse_file(const std::string& path) {
    Scanner sc;

    tokens = sc.get_tokens(path);
    current_token_idx = 0;

    auto mod = parse_module();
    return mod;
}

Ast* Parser::parse_module() {
    AstN* mod = new AstN(AST_MODULE);

    while (true) {
        if (lookahead(TK_IMPORT)) {
            mod->add_child(parse_import());
        } else if (lookahead(TK_DEF)) {
            mod->add_child(parse_function());
        } else if (lookahead(TK_EOF)) {
            break;
        } else {
            std::cout << "unexpected token while parsing\n";
        }
    }

    std::cout << mod->to_json();
    return mod;
}

Ast* Parser::parse_import() {
    Ast* alias = nullptr;

    if (!match(TK_IMPORT)) {
        return nullptr;
    }

    auto path = parse_import_path();

    if (path == nullptr) {
        return nullptr;
    }

    if (match(TK_AS)) {
        alias = parse_identifier();

        if (alias == nullptr) {
            std::cout << "expected an alias for import, but got something else\n";
            delete path;
            return nullptr;
        } else {
            alias->set_kind(AST_IMPORT_ALIAS);
        }
    }

    AstN* node = new AstN(AST_IMPORT);
    node->add_child(path);
    node->add_child(alias);
    return node;
}

Ast* Parser::parse_import_path() {
    Ast* path = parse_identifier();

    if (path == nullptr) {
        std::cout << "log_error: expected an identifer in import path, but got something else\n";
        return nullptr;
    }

    AstN* node = new AstN(AST_IMPORT_PATH);
    node->add_child(path);

    while (match(TK_DOT)) {
        path = parse_identifier();

        if (path) {
            node->add_child(path);
        } else {
            std::cout << "log_error: expected an identifer in import path after '.', but got something else\n";
            delete node;
            return nullptr;
        }
    }

    return node;
}

Ast* Parser::parse_function() {
    if (!match(TK_DEF)) {
        return nullptr;
    }

    auto node = new AstN(AST_DEF);
    auto name = parse_identifier();

    if (name == nullptr) {
        std::cout << "Error: missing name in function definition\n";
        delete node;
        return nullptr;
    }

    node->add_child_as(AST_DEF_NAME, name);

    auto type_parameters = parse_type_parameters();
    node->add_child(type_parameters);

    return node;
}

Ast* Parser::parse_identifier() {
    if (match(TK_IDENTIFIER)) {
        Ast* node = new Ast(AST_IDENTIFIER);
        node->set_token(matched);
        return node;
    }

    return nullptr;
}

Ast* Parser::parse_type_parameters() {
    if (!match(TK_BEGIN_GENERIC)) {
        return nullptr;
    }

    auto node = new AstN(AST_TYPE_PARAMETERS);
    auto id = parse_identifier();

    if (id == nullptr) {
        std::cout << "Error while parsing generic: expected an identifier\n";
        delete node;
        return nullptr;
    }

    node->add_child(id);

    while (match(TK_COMMA)) {
        id = parse_identifier();

        if (id == nullptr) {
            std::cout << "Error: missing identifier after ',' in generics\n";
            delete node;
            return nullptr;
        }

        node->add_child(id);
    }

    if (!match(TK_END_GENERIC)) {
        std::cout << "Error: missing '>' ending generic\n";
        delete node;
        return nullptr;
    }

    return node;
}

bool Parser::match(TokenKind kind) {
    if (lookahead(kind)) {
        matched = tokens[current_token_idx];
        advance();
        return true;
    }

    return false;
}

bool Parser::lookahead(TokenKind kind) {
    if (has_next()) {
        return tokens[current_token_idx].get_kind() == kind;
    }

    return false;
}

bool Parser::has_next() {
    return current_token_idx < tokens.size();
}

void Parser::indent(unsigned int level) {
    indentation_stack.push(level);
}

void Parser::dedent() {
    if (indentation_stack.size() > 0) {
        indentation_stack.pop();
    }
}

bool Parser::is_indented() {
    if (has_next()) {
        auto top = indentation_stack.top();
        auto current = tokens[current_token_idx].get_whitespace();

        return top < current;
    }

    return false;
}

void Parser::advance() {
    if (current_token_idx < tokens.size()) {
        ++current_token_idx;
    }
}
