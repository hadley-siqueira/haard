#include "ast/declaration.h"
#include "ast/module.h"

using namespace haard;

unsigned uid_declaration_counter = 0;

Declaration::Declaration() {
    set_uid(uid_declaration_counter++);
}

const Token& Declaration::get_name() const {
    return name;
}

void Declaration::set_name(const Token& newName) {
    name = newName;
}

Module* Declaration::get_module() const {
    return module;
}

void Declaration::set_module(Module* newModule) {
    module = newModule;
}

const unsigned Declaration::get_uid() const {
    return uid;
}

void Declaration::set_uid(const unsigned newUid) {
    uid = newUid;
}
