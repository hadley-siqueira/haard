#include <haard/name_resolver/use_resolver.h>

using namespace haard;

UseResolver::UseResolver() {
    compilation = nullptr;
    module = nullptr;
    index = 0;
}

void UseResolver::set_compilation(Compilation* compilation) {
    this->compilation = compilation;

    resolver.set_compilation(compilation);
}

void UseResolver::resolve(u32 index) {
    SymbolTable* table;

    this->index = index;
    module = compilation->get_module(index);
    table = module->get_symbols();

    scope_of.clear();
    declarations.clear();
    made.clear();
    local.clear();
    declaring.clear();
    query.set_module(module);

    for (u32 scope = 1; scope < table->get_scope_count(); scope++) {
        if (scope == table->get_module_scope()) {
            continue;
        }

        for (u32 symbol = table->get_scope(scope)->symbols; symbol != 0;
             symbol = table->get_symbol(symbol)->sibling_or_next) {
            for (u32 candidate = table->get_symbol(symbol)->candidates;
                 candidate != 0;
                 candidate = table->get_candidate(candidate)->next_candidate) {
                Candidate* one = table->get_candidate(candidate);
                AstNodeKind kind = kind_of(one->ast_node);

                if (one->kind != SYMBOL_VARIABLE) {
                    continue;
                }

                if (kind == AST_BINDING || kind == AST_ASSIGNMENT) {
                    local.insert(candidate);
                }

                if (kind == AST_ASSIGNMENT) {
                    declaring.insert(one->ast_node);
                }
            }
        }
    }

    // Scope::owner read backwards. The collector stamped every scope with the
    // node that opened it, so this is the same description of the shape and
    // not a second one
    for (u32 scope = 1; scope < table->get_scope_count(); scope++) {
        u32 owner = table->get_scope(scope)->owner;

        if (owner != 0) {
            scope_of[owner] = scope;
        }
    }

    walk(module->get_ast()->get_root(), table->get_module_scope());
}

void UseResolver::walk(u32 node, u32 scope) {
    if (node == 0 || declarations.count(node) > 0) {
        return;
    }

    auto opened = scope_of.find(node);

    if (opened != scope_of.end()) {
        scope = opened->second;
    }

    switch (kind_of(node)) {
    // every declaration wraps its name in one of these, so one rule covers
    // all of them and whatever is added next in the same shape
    case AST_BINDING_NAME:
    case AST_GENERIC_PARAMETERS:
        return;

    // a namespace this table does not model. Neither is resolved, and neither
    // is reported as missing
    case AST_LABEL:
    case AST_GOTO:
        return;

    case AST_IDENTIFIER:
        use(node, scope);
        return;

    // what a declaration is given is read before the name is in view, which
    // is exactly what makes 'let x = x + 1' a use of nothing
    case AST_LET_DECLARATION:
    case AST_CONST_DECLARATION: {
        for (u32 child = module->get_ast()->get_node(node)->get_children();
             child != 0;
             child = module->get_ast()->get_node(child)->get_sibling()) {
            walk(child, scope);
            made.insert(child);
        }

        return;
    }

    // Record 0027: the first 'n = 1' declares n. Its right side is read
    // first, and the name on the left is the declaration and not a use of it
    case AST_ASSIGNMENT:
        if (declaring.count(node) > 0) {
            u32 left = module->get_ast()->get_node(node)->get_children();

            walk(module->get_ast()->get_node(left)->get_sibling(), scope);
            made.insert(node);
            walk(left, scope);
            return;
        }

        break;

    case AST_SCOPE:
        walk_scope(node, scope);
        return;

    // A name where a type is written has to name one. It was resolved like
    // any other use until 2026-09-29, so 'let a : x' with 'x' a variable
    // passed, and 'sizeof(x)' reached the emitter with nothing built. The
    // generic arguments are types of their own and are walked as such
    case AST_NAMED_TYPE: {
        u32 name = module->get_ast()->get_node(node)->get_children();

        if (kind_of(name) == AST_SCOPE) {
            walk_scope(name, scope);
            require_a_type(name, scope);
        } else if (kind_of(name) == AST_IDENTIFIER) {
            use(name, scope);
            require_a_type(name, scope);
        }

        walk(module->get_ast()->get_node(name)->get_sibling(), scope);
        return;
    }

    // its loop variables are declarations and the rest of the head is what it
    // reads. They are noted before the head is walked, so the walk below skips
    // them and resolves the sequence
    case AST_FOR_EACH:
        for (u32 variable : query.get_loop_variables(node)) {
            declarations.insert(variable);
        }

        break;

    // The pattern of a 'case' is not a name in this scope: it is a variant of
    // whatever the switch's subject turned out to be, which is the same rule
    // the right side of a '.' answers to and for the same reason. The
    // StatementChecker asks the subject's type, where the answer is.
    //
    // Its block is walked, since what is written inside one is ordinary code
    case AST_CASE:
        walk(module->get_ast()->get_node(
                 module->get_ast()->get_node(node)->get_children())
                 ->get_sibling(),
             scope);
        return;

    // the right side is a member of whatever the left side is, so it waits on
    // a type. Only the left side is a name in this scope.
    //
    // Both operators, and forgetting the second one is exactly the silent bug
    // this rule exists to prevent: 'p->value' would resolve 'value' through
    // the scope chain and find a global of that name
    case AST_ARROW:
    case AST_DOT:
        walk(module->get_ast()->get_node(node)->get_children(), scope);
        return;

    // The key of a brace literal is not a name in this scope, whichever of the
    // two things that literal turns out to be. Hadley, 2026-09-03: with a type
    // written it initialises that type's fields and the key is a **field
    // name**, checked against the declaration; with none it builds an
    // anonymous struct and the key **is** the field it declares. Neither is a
    // variable being read, so neither is this walk's business -- and agenda
    // 1.23 can settle the rest without changing this line.
    //
    // Only the key is skipped: the value is an ordinary expression and every
    // name in it is a name in this scope
    case AST_HASH_PAIR:
        walk(module->get_ast()->get_node(
                 module->get_ast()->get_node(node)->get_children())
                 ->get_sibling(),
             scope);
        return;

    default:
        break;
    }

    for (u32 child = module->get_ast()->get_node(node)->get_children();
         child != 0; child = module->get_ast()->get_node(child)->get_sibling()) {
        walk(child, scope);
    }
}

void UseResolver::walk_scope(u32 node, u32 scope) {
    u32 first = module->get_ast()->get_node(node)->get_children();
    u32 second = module->get_ast()->get_node(first)->get_sibling();

    // one child is '::name', two are 'alias::name' -- AstBuilder::make_scope
    if (second == 0) {
        if (resolver.resolve_at_module(index, text_of(first)).size() == 0) {
            report(first, "cannot find '" + text_of(first) +
                   "' in the module scope");
        }

        return;
    }

    if (resolver.resolve_qualified(index, text_of(first), text_of(second))
            .size() == 0) {
        report(second, "cannot find '" + text_of(second) + "' in '" +
               text_of(first) + "'");
    }
}

void UseResolver::use(u32 node, u32 scope) {
    std::string name = text_of(node);
    std::vector<Candidacy> found = resolver.resolve(index, scope, name);

    if (found.size() > 0) {
        // the name a local is resolved to, read before its statement ended.
        // Until 2026-09-24 this went through in silence, and the emitter
        // found a binding with no type, or g++ a name used before it
        if (found[0].module == index && local.count(found[0].candidate) > 0
            && made.count(module->get_symbols()
                              ->get_candidate(found[0].candidate)->ast_node)
                   == 0) {
            report(node, "'" + name + "' is used before it is declared");
        }

        return;
    }

    report(node, "cannot find '" + name + "' in this scope");
}

// The same question TypeBuilder::type_symbol asks, so the two cannot disagree:
// a name is a type when any declaration it reaches is one. Reaching nothing
// was already reported, by 'use' or 'walk_scope'
void UseResolver::require_a_type(u32 name, u32 scope) {
    std::vector<Candidacy> found;
    u32 at = name;

    if (kind_of(name) == AST_SCOPE) {
        u32 first = module->get_ast()->get_node(name)->get_children();
        u32 second = module->get_ast()->get_node(first)->get_sibling();

        at = second == 0 ? first : second;
        found = second == 0
                    ? resolver.resolve_at_module(index, text_of(first))
                    : resolver.resolve_qualified(index, text_of(first),
                                                 text_of(second));
    } else {
        found = resolver.resolve(index, scope, text_of(name));
    }

    if (found.size() == 0) {
        return;
    }

    for (const Candidacy& one : found) {
        switch ((SymbolKind) compilation->get_module(one.module)
                    ->get_symbols()->get_candidate(one.candidate)->kind) {
        case SYMBOL_CLASS:
        case SYMBOL_STRUCT:
        case SYMBOL_ENUM:
        case SYMBOL_UNION:
        case SYMBOL_GENERIC:
            return;

        default:
            break;
        }
    }

    report(at, "'" + text_of(at) + "' is not a type");
}

void UseResolver::report(u32 node, const std::string& message) {
    Token& token = module->get_tokens()->get_token(
        module->get_ast()->get_node(node)->get_token());

    module->get_logger()->error(token.get_offset(), token.get_length(),
                                message);
}

std::string UseResolver::text_of(u32 node) {
    return std::string(module->get_token_value(
        module->get_ast()->get_node(node)->get_token()));
}

AstNodeKind UseResolver::kind_of(u32 node) {
    return (AstNodeKind) module->get_ast()->get_node(node)->get_kind();
}
