#include <haard/scanner/scanner.h>
#include <iostream>
#include <unordered_map>

using namespace haard;

TokenKind get_token_kind(const std::string& lexeme) {
    static const std::unordered_map<std::string, TokenKind> table = {
        {"import", TK_IMPORT},
        {"def", TK_DEF},
        {"class", TK_CLASS},
        {"enum", TK_ENUM},
        {"union", TK_UNION},
        {"struct", TK_STRUCT},
        {"if", TK_IF},
        {"elif", TK_ELIF},
        {"else", TK_ELSE},
        {"for", TK_FOR},
        {"while", TK_WHILE},
        {"true", TK_TRUE},
        {"false", TK_FALSE},
        {"as", TK_AS},
        {"=", TK_ASSIGNMENT},
        {"+", TK_PLUS},
        {"-", TK_MINUS},
        {"*", TK_TIMES},
        {"/", TK_DIVISION},
        {"++", TK_INCREMENT},
        {"--", TK_DECREMENT},
        {"==", TK_EQUAL},
        {"!=", TK_NOT_EQUAL},
        {"<", TK_LESS_THAN},
        {"<=", TK_LESS_THAN_OR_EQUAL},
        {">", TK_GREATER_THAN},
        {">=", TK_GREATER_THAN_OR_EQUAL},
        {".", TK_DOT},
        {":", TK_COLON},
        {"@", TK_AT},
    };

    auto it = table.find(lexeme);

    if (it != table.end()) {
        return it->second;
    }

    return TK_UNKNOWN;
}

Scanner::Scanner() {
    tokens = nullptr;
    source_file = nullptr;
    context = nullptr;
    token_offset = 0;
    token_length = 0;
    idx = 0;
    template_flag = false;
    template_counter = 0;
}

void Scanner::get_tokens(const std::filesystem::path& path) {
    tokens->reset();
    source_file->open(path);

    while (has_next()) {
        get_token();
    }
}

void Scanner::get_token() {
    if (is_newline()) {
        advance();
    } else if (is_alpha()) {
        get_keyword_or_identifier();
    } else if (is_digit()) {
        get_number();
    } else if (is_operator()) {
        get_operator();
    } else if (is_newline()) {
        advance();
    } else {
        advance();
    }
}

void Scanner::get_keyword_or_identifier() {
    start_token();

    while (is_alphanum()) {
        advance();
    }

    end_token();
    auto lexeme = get_lexeme_from_token();
    auto kind = get_token_kind(lexeme);

    if (kind == TK_UNKNOWN) {
        kind = TK_IDENTIFIER;
    }

    create_token(kind);

    if (lookahead('<')) {
        template_flag = true;
    }
}

void Scanner::get_number() {
    auto kind = TK_INTEGER_LITERAL;

    start_token();

    if (lookahead("0b")) {
        advance(2);

        if (is_binary_digit()) {
            while (is_binary_digit() || lookahead('_')) {
                advance();
            }
        } else {
            std::cout << "Error: missing binary digits after '0b'\n";
            auto v = source_file->get_lines_by_index(idx, 0, 0);
            std::cout << v << std::endl;
            // log_error(file_id, token_offset, ERR_MISSING_BINARY_DIGITS)
        }
    } else if (lookahead("0o")) {
        advance(2);

        if (is_octal_digit()) {
            while (is_octal_digit() || lookahead('_')) {
                advance();
            }
        } else {
            std::cout << "Error: missing octal digits after '0o'\n";
        }
    } else if (lookahead("0x")) {
        advance(2);

        if (is_hex_digit()) {
            while (is_hex_digit() || lookahead('_')) {
                advance();
            }
        } else {
            std::cout << "Error: missing hexadecimal digits after '0x'\n";
        }
    } else {
        while (is_digit() || lookahead('_')) {
            advance();
        }

        if (lookahead('.') && is_digit(1) || lookahead('.', 1)) {

        }
    }

    end_token();
    create_token(kind);
}

void Scanner::get_operator() {
    std::string tmp;

    for (int i = 0; i < 4; ++i) { 
        tmp += source_file->char_at(idx + i);
    }

    while (tmp.size() > 0 && get_token_kind(tmp) == TK_UNKNOWN) {
        tmp.pop_back();
    }

    if (tmp.size() == 0) {
        std::cout << "Error: Something went wrong while scanning an operator\n";
        return;
    }

    auto kind = get_token_kind(tmp);

    if (template_flag || template_counter > 0) {
        if (lookahead('<')) {
            kind = TK_BEGIN_GENERIC;
            ++template_counter;
            tmp = "<";
        } else if (lookahead('>')) {
            kind = TK_END_GENERIC;
            --template_counter;
            tmp = ">";
        }
    }

    template_flag = false;
    start_token();
    advance(tmp.size());
    end_token();
    create_token(kind);
}

void Scanner::set_context(Context* context) {
    this->context = context;
    this->source_file = context->get_source_file();
    this->tokens = context->get_tokens();
}

bool Scanner::has_next() {
    return idx < source_file->size();
};

void Scanner::start_token() {
    token_offset = idx;
    token_length = 0;
}

void Scanner::end_token() {
    token_length = idx - token_offset;

    if (token_length >= 65535) {
        std::cout << "Error while scanning: token too long\n";
    }
}

void Scanner::create_token(TokenKind kind) {
    Token token;

    token.set_kind(kind);
    token.set_offset(token_offset);
    token.set_length(token_length);

    tokens->push(token);
}

void Scanner::advance(int steps) {
    for (int i = 0; i < steps; ++i) {
        advance();
    }
}

void Scanner::advance() {
    char c = source_file->char_at(idx);

    if (c == '\n') {
        column = 1;
        line++;
        ws = 0;
        line_start = true;
    } else if (c == ' ' && line_start) {
        ws++;
    } else if (((c >> 7) & 1) == 0) {
        column++;
        //value += c;
    } else if (((c >> 6) & 0b11) == 0b10) {
        //value += c;
    } else if (((c >> 5) & 0b111) == 0b110) {
        column++;
        //value += c;
    } else if (((c >> 4) & 0b1111) == 0b1110) {
        column++;
        //value += c;
    } else if (((c >> 3) & 0b11111) == 0b11110) {
        column++;
        //value += c;
    } else {
        std::cout << "Error: unknown char = " << ((int) c) << '\n';
    }

    ++idx;
}

bool Scanner::lookahead(char c) {
    return has_next() && source_file->char_at(idx) == c;
}

bool Scanner::lookahead(char c, int offset) {
    return source_file->char_at(idx + offset) == c;
}

bool Scanner::lookahead(const char* s) {
    int offset = 0;

    while (*s != '\0') {
        if (!lookahead(*s, offset)) {
            return false;
        }

        ++offset;
        ++s;
    }

    return true;
}

bool Scanner::is_newline() {
    return source_file->char_at(idx) == '\n';
}

bool Scanner::is_alpha(int offset) {
    char c = source_file->char_at(idx + offset);

    return c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c == '_' || c >= 128;
}

bool Scanner::is_digit(int offset) {
    char c = source_file->char_at(idx + offset);

    return c >= '0' && c <= '9';
}

bool Scanner::is_binary_digit(int offset) {
    char c = source_file->char_at(idx + offset);

    return c >= '0' && c <= '1';
}

bool Scanner::is_octal_digit(int offset) {
    char c = source_file->char_at(idx + offset);

    return c >= '0' && c <= '7';
}

bool Scanner::is_hex_digit(int offset) {
    char c = source_file->char_at(idx + offset);

    return c >= '0' && c <= '9' || c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z';
}

bool Scanner::is_alphanum(int offset) {
    return is_alpha(offset) || is_digit(offset);
}

bool Scanner::is_operator(int offset) {
    char c = source_file->char_at(idx + offset);

    return c == '(' || c == ')' || c == '[' || c == ']'
        || c == '{' || c == '}' || c == '+' || c == '-'
        || c == '*' || c == '/' || c == '%' || c == '!'
        || c == '&' || c == '|' || c == '~' || c ==  '='
        || c == '>' || c == '<' || c == '^' || c == '.'
        || c == '$' || c == ':' || c == '?' || c == '@'
        || c == ',' || c == ';';
}

std::string Scanner::get_lexeme_from_token() {
    std::string r;

    for (auto i = 0; i < token_length; ++i) {
        r += source_file->char_at(token_offset + i);
    }

    return r;
}
