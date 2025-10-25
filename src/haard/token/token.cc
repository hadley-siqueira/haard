#include <haard/token/token.h>

using namespace haard;

void Token::set_offset(unsigned int offset) {
    this->offset = offset;
}

void Token::set_line(unsigned int line) {
    this->line = line;
}

void Token::set_column(unsigned int column) {
    this->column = column;
}

void Token::set_whitespace(unsigned int whitespace) {
    this->whitespace = whitespace;
}

void Token::set_value(char* value) {
    this->value = value;
}

unsigned int Token::get_offset() {
    return offset;
}

unsigned int Token::get_line() {
    return line;
}

unsigned int Token::get_column() {
    return column;
}

unsigned int Token::get_whitespace() {
    return whitespace;
}

char* Token::get_value() {
    return value;
}
