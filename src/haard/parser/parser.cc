#include <iostream>
#include <sstream>
#include <fstream>

#include <haard/parser/parser.h>
#include <haard/utils/utils.h>
#include <haard/scanner/scanner.h>

using namespace haard;

Parser::Parser() {
    logger = nullptr;
    idx = 0;
}

Parser::~Parser() {

}

Module* Parser::read(const std::string& path) {
    Scanner sc;
    Module* mod;

    idx = 0;
    tokens = sc.read(path);
    this->path = path;
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
        log_error_missing_import_path();
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
            log_error_missing_import_alias();
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

std::string get_line_from_file(std::string path, unsigned line) {
    char c;
    unsigned count = 1;
    std::ifstream file;
    std::string buffer;

    file.open(path);

    while (count != line && file.get(c)) {
        if (c == '\n') {
            ++count;
        }
    }

    while (file.get(c)) {
        if (c == '\n') {
            break;
        } else {
            buffer += c;
        }
    }

    file.close();
    return buffer;
}


void Parser::log_error_missing_import_path() {
    unsigned line = matched.get_line();
    unsigned column = matched.get_column();

    if (logger == nullptr) {
        return;
    }

    std::stringstream ss;
    ss << "missing path while parsing import:\n";
    ss << "--> " << path << ":" << line << ":" << column << '\n';
    auto lin = get_line_from_file(path, matched.get_line());
    auto exp = add_explanation(lin, "expected a path here", column + 7);
    ss << exp;

    logger->error(ss.str());
}


void Parser::log_error_missing_import_alias() {
    unsigned line = matched.get_line();
    unsigned column = matched.get_column();

    if (logger == nullptr) {
        return;
    }

    std::stringstream ss;
    ss << "expected an alias after token 'as' while parsing import:\n";
    ss << "--> " << path << ":" << line << ":" << column << '\n';
    auto lin = get_line_from_file(path, matched.get_line());
    auto exp = add_explanation(lin, "expected an alias here", column + 3);
    ss << exp;

    logger->error(ss.str());
}
