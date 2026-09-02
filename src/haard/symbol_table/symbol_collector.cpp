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
