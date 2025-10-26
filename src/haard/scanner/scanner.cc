#include <iostream>
#include <fstream>
#include <unordered_map>

#include <haard/scanner/scanner.h>

using namespace haard;

Scanner::Scanner() {
    reset_state();
}

std::vector<Token> Scanner::get_tokens(const std::string& path) {
    reset_state();
    read_to_buffer(path);

    while (has_tokens()) {
        if (is_whitespace() || is_newline()) {
            advance();
        } else if (is_alpha()) {
            get_keyword_or_identifier();
        }
    }

    return tokens;
}

void Scanner::get_keyword_or_identifier() {
    start_token();

    while (is_alphanum()) {
        advance();
    }

    if (is_keyword(value)) {

    }

    create_token();
}

bool Scanner::is_keyword(const std::string& v) {
    std::unordered_map<std::string, TokenKind> mapping;
    return true;
}

void Scanner::read_to_buffer(const std::string& path) {
    std::ifstream file(path);
    char c;

    while (file.get(c)) {
        buffer += c;
    }
}

void Scanner::reset_state() {
    ws = 0;
    line = 1;
    column = 1;
    offset = 0;
    idx = 0;
    buffer = "";
    line_start = true;
    tokens.clear();
}

bool Scanner::has_tokens() {
    return idx < buffer.size();
}

char Scanner::advance() {
    char c = buffer[idx];

    if (c == '\n') {
        column = 1;
        line++;
        ws = 0;
        line_start = true;
    } else if (c == ' ' && line_start) {
        ws++;
    } else if (((c >> 7) & 1) == 0) {
        column++;
        value += c;
    } else if (((c >> 6) & 0b11) == 0b10) {
        value += c;
    } else if (((c >> 5) & 0b111) == 0b110) {
        column++;
        value += c;
    } else if (((c >> 4) & 0b1111) == 0b1110) {
        column++;
        value += c;
    } else if (((c >> 3) & 0b11111) == 0b11110) {
        column++;
        value += c;
    }
    std::cout << idx << ' ' << buffer.size() << '\n';
    ++idx;
 /*   if (idx < buffer.size()) {
        idx++;
    }*/

    return c;
}

bool Scanner::is_whitespace() {
    char c = buffer[idx];

    return c == ' ' || c == '\t';
}

bool Scanner::is_newline() {
    return buffer[idx] == '\n';
}

bool Scanner::is_alpha() {
    char c = buffer[idx];

    return c >= 'a' && c <= 'z' 
        || c >= 'A' && c <= 'Z' 
        || c == '_'
        || c >= 128;
}

bool Scanner::is_digit() {
    char c = buffer[idx];

    return c >= '0' && c <= '9';
}

bool Scanner::is_alphanum() {
    return is_alpha() || is_digit();
}

void Scanner::create_token() {
    token.set_value(StringPool::get(value));
    tokens.push_back(token);
}

void Scanner::start_token() {
    token.set_line(line);
    token.set_column(column);
    token.set_whitespace(ws);
    token.set_offset(offset);
    value = "";
}
