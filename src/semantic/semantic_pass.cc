#include "semantic/semantic_pass.h"

using namespace haard;

void SemanticPass::enter_scope(SymbolTable* scope) {
    scope_stack.push(current_scope);
    current_scope = scope;
}

void SemanticPass::leave_scope() {
    current_scope = scope_stack.top();
    scope_stack.pop();
}

Symbol* SemanticPass::resolve(std::string name) {
    return current_scope->resolve(name);
}

Logger* SemanticPass::get_logger() const {
    return logger;
}

void SemanticPass::set_logger(Logger* newLogger) {
    logger = newLogger;
}
