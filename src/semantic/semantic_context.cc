#include "semantic/semantic_context.h"

using namespace haard;

SemanticContext::SemanticContext() {
    set_module(nullptr);
    set_class(nullptr);
    set_union(nullptr);
    set_enum(nullptr);
    set_struct(nullptr);
}

Module* SemanticContext::get_module() const {
    return module;
}

void SemanticContext::set_module(Module* newModule) {
    module = newModule;
}

Class* SemanticContext::get_class() const {
    return klass;
}

void SemanticContext::set_class(Class* newKlass) {
    klass = newKlass;
}

Union* SemanticContext::get_union() const {
    return union_;
}

void SemanticContext::set_union(Union* newUnion) {
    union_ = newUnion;
}

Enum* SemanticContext::get_enum() const {
    return enum_;
}

void SemanticContext::set_enum(Enum* newEnum) {
    enum_ = newEnum;
}

Struct* SemanticContext::get_struct() const {
    return struct_;
}

void SemanticContext::set_struct(Struct* newStruct) {
    struct_ = newStruct;
}

Function* SemanticContext::get_function() const {
    return function;
}

void SemanticContext::set_function(Function* newFunction) {
    function = newFunction;
}

Scope* SemanticContext::get_scope() const {
    return scope;
}

void SemanticContext::set_scope(Scope* newScope) {
    scope = newScope;
}

void SemanticContext::push_scope(Scope* scope) {
    scope_stack.push(this->scope);
    this->scope = scope;
}
