#include <haard/compilation/compilation.h>
#include <haard/string_table/string_table.h>
#include <haard/symbol_table/implicit_collector.h>

using namespace haard;

ImplicitCollector::ImplicitCollector() {
    compilation = nullptr;
    module = nullptr;
    index = 0;
}

void ImplicitCollector::set_compilation(Compilation* compilation) {
    this->compilation = compilation;

    names.set_compilation(compilation);
}

bool ImplicitCollector::declare(u32 index) {
    this->index = index;
    module = compilation->get_module(index);

    query.set_module(module);

    SymbolTable* table = module->get_symbols();
    std::vector<u32> declarations = query.get_declarations();
    u32 done = walked.count(index) > 0 ? walked[index] : 0;

    if (done >= declarations.size()) {
        return false;
    }

    scope_of.clear();

    for (u32 scope = 1; scope < table->get_scope_count(); scope++) {
        u32 owner = table->get_scope(scope)->owner;

        if (owner != 0) {
            scope_of[owner] = scope;
        }
    }

    // declaring a name opens no scope, so the map above stays true for the
    // whole walk however many names it adds
    for (u32 i = done; i < declarations.size(); i++) {
        walk(declarations[i], table->get_module_scope());
    }

    walked[index] = (u32) declarations.size();

    return true;
}

void ImplicitCollector::walk(u32 node, u32 scope) {
    if (node == 0) {
        return;
    }

    auto opened = scope_of.find(node);

    if (opened != scope_of.end()) {
        scope = opened->second;
    }

    // source order is the whole of it: the first 'n = 1' declares and every
    // one after it finds what the first left behind
    if (module->get_ast()->get_node(node)->get_kind() == AST_ASSIGNMENT) {
        declare_target(scope, node);
    }

    for (u32 child : query.get_children(node)) {
        walk(child, scope);
    }
}

void ImplicitCollector::declare_target(u32 scope, u32 assignment) {
    Ast* ast = module->get_ast();
    u32 target = ast->get_node(assignment)->get_children();

    if (target == 0
        || ast->get_node(target)->get_kind() != AST_IDENTIFIER) {
        return;
    }

    std::string name =
        std::string(module->get_token_value(ast->get_node(target)->get_token()));

    if (names.resolve_in_module(index, scope, name).size() > 0) {
        return;
    }

    // the candidate points at the ASSIGNMENT and not at the identifier, the
    // same way a 'let' points at its binding: from the name alone there is no
    // way back to the value it was given, and that value is what the type
    // phase infers the name's type from
    module->get_symbols()->declare(scope, module->get_strings()->intern(name),
                                   SYMBOL_VARIABLE, assignment);
}
