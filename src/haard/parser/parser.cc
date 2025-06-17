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
            auto imp = parse_import();

            if (imp) {
                mod->add_import(parse_import());
            } else {
                recover();
            }
        } else if (lookahead(TK_LET) || lookahead(TK_CONST)) {
            auto var = parse_variable();
            
            if (var) {
                mod->add_variable(var);
            } else {
                recover();
            }
        } else if (lookahead(TK_DEF)) {
            auto function = parse_function();

            if (function) {
                mod->add_function(function);
            } else {
                recover();
            }
        } else if (match(TK_EOF)) {
            break;
        } else {
            match();
            log_error_unexpected_token();
            delete mod;
            return nullptr;
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

Variable* Parser::parse_variable() {
    Variable* var = nullptr;
    Type* type = nullptr;
    Expression* expression = nullptr;
    bool const_flag = false;
    Token name;

    if (!(lookahead(TK_LET) || lookahead(TK_CONST))) {
        return nullptr;
    }

    if (match(TK_LET)) {
        const_flag = false;
    } else if (match(TK_CONST)) {
        const_flag = true;
    }

    if (match(TK_ID)) {
        name = matched;
    } else {
        if (const_flag) {
            std::cout << "Error: expected constant name\n";
        } else {
            std::cout << "error: expected variable name\n";
        }

        return nullptr;
    }

    if (match(TK_COLON)) {
        type = parse_type();

        if (type == nullptr) {
            if (const_flag) {
                std::cout << "error: missing type after colon on constant\n";
            } else {
                std::cout << "error: missing type after colon on variable\n";
            }

            return nullptr;
        }
    }

    if (match(TK_ASSIGNMENT)) {
        expression = parse_expression();

        if (expression == nullptr) {
            if (const_flag) {
                std::cout << "error: missing expression on constant\n";
            } else {
                std::cout << "error: missing expression on variable\n";
            }

            delete type;
            return nullptr;
        }
    }

    if (type == nullptr && expression == nullptr) {
        std::cout << "error: expected at least a type or expression on declaration\n";
        return nullptr;
    }

    var = new Variable();
    var->set_name(name);
    var->set_type(type);
    var->set_expression(expression);
    var->set_const(const_flag);

    return var;
}

Function* Parser::parse_function() {
    if (!lookahead(TK_DEF)) {
        return nullptr;
    }

    match(TK_DEF);

    if (!match(TK_ID)) {
        std::cout << "error: missing function name after token 'def'\n";
        return nullptr;
    }

    auto name = matched;

    if (!match(TK_COLON)) {
        std::cout << "error: missing ':' on function definition\n";
        return nullptr;
    }

    auto return_type = parse_type();

    if (return_type == nullptr) {
        std::cout << "error: missing return type after ':'\n";
        return nullptr;
    }

    auto function = new Function();

    indent();

    while (is_indented() && lookahead(TK_AT)) {
        auto param = parse_parameter();

        if (param) {
            function->add_parameter(param);
        }
    }

    dedent();

    function->set_name(name);
    function->set_return_type(return_type);

    return function;
}

Variable* Parser::parse_parameter() {
    Variable* var = nullptr;
    Type* type = nullptr;
    Expression* expression = nullptr;
    bool const_flag = false;
    Token name;

    match(TK_AT);

    if (!match(TK_ID)) {
        std::cout << "error: expected parameter name\n";
        return nullptr;
    } else {
        name = matched;
    }

    if (!match(TK_COLON)) {
        std::cout << "error: missing ':' on parameter definition\n";
        return nullptr;
    }

    type = parse_type();

    if (type == nullptr) {
        std::cout << "error: missing type after colon on parameter\n";
        return nullptr;
    }

    if (match(TK_ASSIGNMENT)) {
        expression = parse_expression();

        if (expression == nullptr) {
            std::cout << "error: missing expression after '=' on parameter definition\n";
            delete type;
            return nullptr;
        }
    }

    var = new Variable();
    var->set_name(name);
    var->set_type(type);
    var->set_expression(expression);

    return var;

}

Type* Parser::parse_type() {
    Type* type = nullptr;

    if (match(TK_I8) || match(TK_U8)) {
        type =  new PrimitiveType(matched);
    } else if (match(TK_I16) || match(TK_U16)) {
        type =  new PrimitiveType(matched);
    } else if (match(TK_I32) || match(TK_U32)) {
        type =  new PrimitiveType(matched);
    } else if (match(TK_I64) || match(TK_U64)) {
        type =  new PrimitiveType(matched);
    } else if (match(TK_F64) || match(TK_F32)) {
        type =  new PrimitiveType(matched);
    } else if (match(TK_BOOL) || match(TK_CHAR)) {
        type =  new PrimitiveType(matched);
    } else if (match(TK_STR) || match(TK_SYMBOL)) {
        type =  new PrimitiveType(matched);
    } else if (match(TK_VOID)) {
        type =  new PrimitiveType(matched);
    }

    bool tail = true;

    while (tail) {
        tail = false;

        if (match(TK_TIMES)) {
            type = new BoxedType(type, matched);
            tail = true;
        } else if (match(TK_BITWISE_AND)) {
            type = new BoxedType(type, matched);
            tail = true;
        } else if (match(TK_LOGICAL_AND)) {
            type = new BoxedType(type, matched);
            tail = true;
        } else if (match(TK_POWER)) {
            type = new BoxedType(type, matched);
            type = new BoxedType(type, matched);
            tail = true;
        }
    }

    return type;
}

Expression* Parser::parse_expression() {
    if (match(TK_LITERAL_CHAR) || match(TK_LITERAL_INTEGER)) {
        return new Literal(matched);
    } else {

    }

    return nullptr;
}

void Parser::recover() {
    bool flag = true;
    bool found = false;

    while (flag) {
        found = false;
        found = found || lookahead(TK_DEF);
        found = found || lookahead(TK_LET);
        found = found || lookahead(TK_CONST);
        found = found || lookahead(TK_IMPORT);
        found = found || lookahead(TK_CLASS);
        found = found || lookahead(TK_UNION);
        found = found || lookahead(TK_STRUCT);
        found = found || lookahead(TK_ENUM);
        found = found || lookahead(TK_EOF);

        if (found) {
            flag = false;
        } else {
            advance();
        }
    }
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

void Parser::log_error_unexpected_token() {
    unsigned line = matched.get_line();
    unsigned column = matched.get_column();

    if (logger == nullptr) {
        return;
    }

    std::stringstream ss;
    ss << "unexpected token while parsing a module\n";
    ss << "--> " << path << ":" << line << ":" << column << '\n';
    auto lin = get_line_from_file(path, matched.get_line());
    auto exp = add_explanation(lin, "expected a function, class etc. But got something else", column);
    ss << exp;

    logger->error(ss.str());
 
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
