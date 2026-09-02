#include <haard/string_table/string_table.h>
#include <haard/symbol_table/symbol_collector.h>

using namespace haard;

SymbolCollector::SymbolCollector() {
    module = nullptr;
}

void SymbolCollector::set_module(Module* module) {
    this->module = module;

    query.set_module(module);
}

void SymbolCollector::collect() {
    u32 scope = module->get_symbols()->get_module_scope();

    // one walk, in source order, and not one pass per kind. The order is what
    // a scope enumerates in, and record 0009 resolves a bare name to the first
    // declaration that answers -- so collecting every function before every
    // class would put a name list in an order the source never wrote
    for (u32 node : query.get_declarations()) {
        switch (module->get_ast()->get_node(node)->get_kind()) {
        case AST_FUNCTION:
            collect_function(scope, node);
            break;

        case AST_CLASS:
            collect_type(scope, node, SYMBOL_CLASS);
            break;

        case AST_STRUCT:
            collect_type(scope, node, SYMBOL_STRUCT);
            break;

        case AST_ENUM:
            collect_type(scope, node, SYMBOL_ENUM);
            break;

        case AST_UNION:
            collect_type(scope, node, SYMBOL_UNION);
            break;

        default:
            collect_binding(scope, node);
            break;
        }
    }

    // and only now the names an assignment declares by being written, because
    // deciding that 'n = 1' declares n means knowing that nothing else in this
    // module is called n -- including a global written further down the file
    SymbolTable* table = module->get_symbols();

    scope_of.clear();

    for (u32 index = 1; index < table->get_scope_count(); index++) {
        u32 owner = table->get_scope(index)->owner;

        if (owner != 0) {
            scope_of[owner] = index;
        }
    }

    collect_implicit(module->get_ast()->get_root(), table->get_module_scope());
}

void SymbolCollector::collect_implicit(u32 node, u32 scope) {
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
        collect_implicit(child, scope);
    }
}

void SymbolCollector::declare_target(u32 scope, u32 assignment) {
    u32 target = module->get_ast()->get_node(assignment)->get_children();

    // one identifier and nothing else. 'a.b', 'p->x' and 'a[i]' all name
    // something that has to exist already, and a tuple target is written with
    // a 'let'
    if (target == 0
        || module->get_ast()->get_node(target)->get_kind() != AST_IDENTIFIER) {
        return;
    }

    std::string name =
        std::string(module->get_token_value(
            module->get_ast()->get_node(target)->get_token()));

    // interning a name that turns out to be there already costs nothing: the
    // table gives back the entry it has
    u32 interned = module->get_strings()->intern(name);

    if (in_view(scope, interned)) {
        return;
    }

    // the candidate points at the ASSIGNMENT and not at the identifier, the
    // same way a 'let' points at its binding: from the name alone there is no
    // way back to the value it was given, and that value is what the type
    // phase infers the name's type from
    module->get_symbols()->declare(scope, interned, SYMBOL_VARIABLE,
                                   assignment);
}

bool SymbolCollector::in_view(u32 scope, u32 name) {
    SymbolTable* table = module->get_symbols();

    for (u32 current = scope; current != 0;
         current = table->get_scope(current)->parent) {
        if (table->find(current, name) != 0) {
            return true;
        }
    }

    return false;
}

void SymbolCollector::collect_type(u32 scope, u32 declaration,
                                   SymbolKind kind) {
    declare(scope, query.get_declaration_name(declaration), kind, declaration);

    u32 body = module->get_symbols()->open_scope(scope, declaration);

    collect_generic_parameters(body, declaration);

    for (u32 member : query.get_members(declaration)) {
        // a method is a 'def' and everything else on a line of its own is a
        // field, which is the same rule Parser::parse_member reads
        SymbolKind member_kind =
            module->get_ast()->get_node(member)->get_kind() == AST_FUNCTION
                ? SYMBOL_FUNCTION
                : SYMBOL_FIELD;

        if (member_kind == SYMBOL_FUNCTION) {
            collect_function(body, member);
        } else {
            declare(body, query.get_declaration_name(member), SYMBOL_FIELD,
                    member);
        }
    }
}

void SymbolCollector::collect_function(u32 scope, u32 declaration) {
    declare(scope, query.get_declaration_name(declaration), SYMBOL_FUNCTION,
            declaration);

    u32 inside = module->get_symbols()->open_scope(scope, declaration);

    collect_generic_parameters(inside, declaration);

    for (u32 param : query.get_params(declaration)) {
        declare(inside, query.get_declaration_name(param), SYMBOL_PARAM,
                param);
    }

    // and the block below it, so a local of a parameter's name shadows the
    // parameter instead of standing beside it as a second candidate
    u32 block = query.get_block(declaration);

    if (block != 0) {
        collect_statement(inside, block);
    }
}

void SymbolCollector::collect_statement(u32 scope, u32 node) {
    u32 inside = scope;

    switch (module->get_ast()->get_node(node)->get_kind()) {
    case AST_LET_DECLARATION:
    case AST_CONST_DECLARATION:
        collect_binding(scope, node);
        break;

    case AST_BLOCK:
        inside = module->get_symbols()->open_scope(scope, node);
        break;

    case AST_CLOSURE:
        collect_closure(scope, node);
        return;

    case AST_FOR_EACH:
        inside = module->get_symbols()->open_scope(scope, node);
        collect_loop_variables(inside, node);
        break;

    // and the C shaped one opens a scope of its own too, for the same reason:
    // its head is where 'for i = 0; ...' declares i, and a loop variable that
    // outlived its loop would be the one place in the language where a name
    // written inside something is still there after it. The head is not in
    // the block -- the block is a child of this node, beside the head -- so
    // the scope has to be opened here and not there
    case AST_FOR:
        inside = module->get_symbols()->open_scope(scope, node);
        break;

    // a declaration written inside a body is not reached from here: the parser
    // only accepts them at the top level, so nothing below is a 'def' or a
    // 'class'
    default:
        break;
    }

    for (u32 child : query.get_children(node)) {
        collect_statement(inside, child);
    }
}

void SymbolCollector::collect_closure(u32 scope, u32 closure) {
    u32 inside = module->get_symbols()->open_scope(scope, closure);

    for (u32 child : query.get_children(closure)) {
        if (module->get_ast()->get_node(child)->get_kind()
            == AST_CLOSURE_PARAMETER) {
            declare(inside, query.get_declaration_name(child), SYMBOL_PARAM,
                    child);
            continue;
        }

        collect_statement(inside, child);
    }
}

void SymbolCollector::collect_loop_variables(u32 scope, u32 for_each) {
    for (u32 variable : query.get_loop_variables(for_each)) {
        AstNode* node = module->get_ast()->get_node(variable);

        // the candidate points at the loop and not at the identifier, for the
        // same reason a tuple binding's names point at their binding: from the
        // name alone there is no way back to the sequence it comes out of
        declare(scope, std::string(module->get_token_value(node->get_token())),
                SYMBOL_VARIABLE, for_each);
    }
}

void SymbolCollector::collect_generic_parameters(u32 scope, u32 declaration) {
    for (u32 parameter : query.get_generic_parameters(declaration)) {
        AstNode* node = module->get_ast()->get_node(parameter);

        declare(scope, std::string(module->get_token_value(node->get_token())),
                SYMBOL_GENERIC, parameter);
    }
}

void SymbolCollector::collect_binding(u32 scope, u32 statement) {
    u32 binding = query.get_binding(statement);

    for (const std::string& name : query.get_binding_names(statement)) {
        declare(scope, name, SYMBOL_VARIABLE, binding);
    }
}

u32 SymbolCollector::declare(u32 scope, const std::string& name,
                             SymbolKind kind, u32 node) {
    u32 interned = module->get_strings()->intern(name);

    return module->get_symbols()->declare(scope, interned, kind, node);
}
