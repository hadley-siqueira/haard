#include <haard/ast/literal.h>

using namespace haard;

Literal::Literal() {
    set_kind(AST_UNKNOWN);
}

Literal::Literal(const Token& token) {
    set_kind(AST_UNKNOWN);
    set_token(token);

    switch (token.get_kind()) {
    case TK_LITERAL_INTEGER:
        set_kind(AST_LITERAL_INTEGER);
        break;

    case TK_LITERAL_CHAR:
        set_kind(AST_LITERAL_CHAR);
        break;

    case TK_LITERAL_FLOAT:
    case TK_LITERAL_DOUBLE:
        set_kind(AST_LITERAL_REAL);
        break;

    case TK_LITERAL_SYMBOL:
        set_kind(AST_LITERAL_SYMBOL);
        break;

    case TK_LITERAL_SINGLE_QUOTE_STRING:
    case TK_LITERAL_DOUBLE_QUOTE_STRING:
        set_kind(AST_LITERAL_STRING);
        break;

    case TK_TRUE:
    case TK_FALSE:
        set_kind(AST_LITERAL_BOOL);
        break;
    }
}

Literal::~Literal() {

}

void Literal::set_token(const Token& token) {
    this->token = token;
}

Token& Literal::get_token() {
    return token;
}
