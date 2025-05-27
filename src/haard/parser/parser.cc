#include <iostream>

#include <haard/parser/parser.h>
#include <haard/scanner/scanner.h>

using namespace haard;

Parser::Parser() {

}

Parser::~Parser() {

}

Module* Parser::read(std::string path) {
    Scanner sc;
    Module* mod;

    idx = 0;
    tokens = sc.read(path);
    mod = parse_module();

    return mod;
}

void Parser::set_logger(Logger* logger) {
    this->logger = logger;
}

Module* Parser::parse_module() {
    Module* mod = new Module();

    while (true) {
        if (lookahead(TK_IMPORT)) {
            mod->add_import(parse_import());
        } else if (match(TK_EOF)) {
            break;
        } else {
            std::cout << "Unexpected token\n";
            exit(0);
        }
    }

    return mod;
}

Import* Parser::parse_import() {
    if (!lookahead(TK_IMPORT)) {
        return nullptr;
    }

    match(TK_IMPORT);
    auto imp = new Import();

    if (!lookahead(TK_ID)) {
        logger->error("missing path in import");
        delete imp;
        return nullptr;
    }

    do {
        match(TK_ID);
        imp->add_to_path(matched);
    } while (match(TK_DOT));

    if (match(TK_AS)) {
        if (match(TK_ID)) {
            imp->set_alias(matched);
        } else {
            std::cout << "Error on parsing import: missing import's alias after 'as' token";
            delete imp;
            return nullptr;
        }
    }

    return imp;
}

void Parser::advance() {
    if (idx < tokens.size()) {
        ++idx;
    }
}

bool Parser::lookahead(TokenKind kind, int offset) {
    if (idx + offset < tokens.size()) {
        return tokens[idx + offset].get_kind() == kind;
    }

    return false;
}

bool Parser::match(TokenKind kind) {
    if (lookahead(kind)) {
        matched = tokens[idx];
        advance();
        return true;
    }

    return false;
}

bool Parser::match_same_line(TokenKind kind) {
    return next_token_on_same_line() && match(kind);
}

bool Parser::match() {
    matched = tokens[idx];
    advance();

    return true;
}

bool Parser::has_next(int offset) {
    return idx + offset < tokens.size();
}

void Parser::indent() {
    indent_stack.push(matched.get_whitespace());
}

void Parser::dedent() {
    indent_stack.pop();
}

bool Parser::is_indented() {
    if (has_next()) {
        return tokens[idx].get_whitespace() > indent_stack.top();
    }

    return false;
}

bool Parser::next_token_on_same_line() {
    if (has_next() && !lookahead(TK_EOF)) {
        return matched.get_line() == tokens[idx].get_line();
    }

    return false;
}


