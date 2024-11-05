#include <sstream>
#include "haard/token/token.h"

using namespace haard;

int Token::get_kind() const {
    return kind;
}

void Token::set_kind(int kind) {
    this->kind = kind;
}

int Token::get_line() const {
    return line;
}

void Token::set_line(int line) {
    this->line = line;
}

int Token::get_column() const {
    return column;
}

void Token::set_column(int column) {
    this->column = column;
}

int Token::get_whitespace() const {
    return whitespace;
}

void Token::set_whitespace(int whitespace) {
    this->whitespace = whitespace;
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

const std::string& Token::get_value() const {
    return value;
}

void Token::set_value(const std::string& value) {
    this->value = value;
}

std::string Token::to_json() {
    std::stringstream ss;

    ss << "{\"kind\": \"" << token_kind_to_str_map.at(get_kind()) << "\", \"line\": "
       << line << ", \"column\": " << column << ", \"whitespace\": " << whitespace
       << "\"value\": \"" << value << "\"";

    return ss.str();
}
