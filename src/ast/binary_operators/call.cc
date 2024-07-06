#include <sstream>
#include "ast/binary_operators/call.h"

using namespace haard;

Call::Call(Expression* object, ExpressionList* arguments) {
    set_kind(AST_CALL);
    set_object(object);
    set_arguments(arguments);
}

Call::Call(Token& token, Expression* object, ExpressionList* arguments) {
    set_kind(AST_CALL);
    set_object(object);
    set_arguments(arguments);
    set_token(token);
}

Call::~Call() {
    delete object;
    delete arguments;
}

Expression* Call::get_object() const {
    return object;
}

void Call::set_object(Expression* value) {
    object = value;
}

ExpressionList* Call::get_arguments() const {
    return arguments;
}

void Call::set_arguments(ExpressionList* value) {
    arguments = value;
}

