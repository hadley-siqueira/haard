#include "semantic/semantic_pass.h"

using namespace haard;

SemanticPass::SemanticPass() {
    set_context(nullptr);
}

void SemanticPass::enter_scope(Scope* scope, bool set_parent) {
    /*context->push_scope(scope);
    scope_stack.push(current_scope);

    if (set_parent) {
        scope->set_parent(current_scope);
    }

    current_scope = scope;*/
}

void SemanticPass::leave_scope() {
    /*current_scope = scope_stack.top();
    scope_stack.pop();*/
}

Symbol* SemanticPass::resolve(std::string name) {
    return context->get_scope()->resolve(name);
}

void SemanticPass::define(SymbolDescriptorKind kind, std::string name, void* descriptor) {
    /*current_scope->define(kind, name, descriptor);*/
}

Logger* SemanticPass::get_logger() const {
    return logger;
}

void SemanticPass::set_logger(Logger* newLogger) {
    logger = newLogger;
}

SemanticContext* SemanticPass::get_context() const {
    return context;
}

void SemanticPass::set_context(SemanticContext* newContext) {
    context = newContext;
}
