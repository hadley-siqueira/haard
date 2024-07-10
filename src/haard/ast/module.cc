#include "haard/ast/module.h"
#include "utils/utils.h"

using namespace haard::ast;

Module::Module() {

}

Module::~Module() {
    for (auto i : imports) {
        delete i;
    }
}

void Module::add_import(Import* import) {
    imports.push_back(import);
}

int Module::imports_count() {
    return imports.size();
}

Import* Module::get_import(size_t idx) {
    return imports[idx];
}

Import* Module::get_import_with_alias(std::string alias) {
    for (int i = 0; i < imports_count(); ++i) {
        Import* imp = get_import(i);

        if (alias == imp->get_alias()) {
            return imp;
        }
    }

    return nullptr;
}
