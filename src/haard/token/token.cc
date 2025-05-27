#include <sstream>
#include "haard/token/token.h"
#include "haard/string_pool/string_pool.h"

using namespace haard;

TokenKind Token::get_kind() const {
    return kind;
}

void Token::set_kind(TokenKind kind) {
    this->kind = kind;
}

unsigned Token::get_line() const {
    return line;
}

void Token::set_line(unsigned line) {
    this->line = line;
}

unsigned Token::get_column() const {
    return column;
}

void Token::set_column(unsigned column) {
    this->column = column;
}

unsigned Token::get_offset() {
    return offset;
}

void Token::set_offset(unsigned offset) {
    this->offset = offset;
}

unsigned Token::get_whitespace() const {
    return whitespace;
}

void Token::set_whitespace(unsigned whitespace) {
    this->whitespace = whitespace;
}

const char* Token::get_value() const {
    return value;
}

void Token::set_value(const char* value) {
    this->value = StringPool::get(value);
}

std::string Token::to_str() {
    std::stringstream ss;

    ss << "(";
    ss << token_kind_to_str_map.at(get_kind()) << ", ";
    ss << get_value() << ", ";
    ss << get_line() << ", ";
    ss << get_column() << ", ";
    ss << get_whitespace() << ")";

    return ss.str();
}

std::string Token::to_json() {
    std::stringstream ss;

    ss << "{\n";
    ss << "    \"kind\": \"" << token_kind_to_str_map.at(get_kind()) << "\",\n";
    ss << "    \"line\": " << get_line() << ",\n";
    ss << "    \"column\": " << get_column() << ",\n";
    ss << "    \"whitespace\": " << get_whitespace() << ",\n";
    ss << "    \"offset\": " << get_offset() << ",\n";

    if (get_value()) {
        ss << "    \"value\": \"" << get_value() << "\"\n";
    }

    ss << "}";

    return ss.str();
}


