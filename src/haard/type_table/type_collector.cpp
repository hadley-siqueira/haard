#include <haard/type_table/type_collector.h>

using namespace haard;

TypeCollector::TypeCollector() {
    compilation = nullptr;
    module = nullptr;
    index = 0;
}

void TypeCollector::set_compilation(Compilation* compilation) {
    this->compilation = compilation;

    builder.set_compilation(compilation);
    typer.set_compilation(compilation);
    coercion.set_compilation(compilation);
}

bool TypeCollector::collect(u32 index) {
    return walk(index, false);
}

bool TypeCollector::infer(u32 index) {
    return walk(index, true);
}

bool TypeCollector::walk(u32 index, bool given) {
    this->index = index;
    module = compilation->get_module(index);

    SymbolTable* table = module->get_symbols();
    std::map<u32, u32>& mark = given ? inferred : collected;
    u32 done = mark.count(index) > 0 ? mark[index] : 0;
    u32 count = table->get_candidate_count();
    bool worked = false;

    // The list grows while it is walked, which is the module loop's shape one
    // level down: building a type may instantiate a generic, and record 0002
    // makes that a cloned declaration with scopes and candidates of its own.
    //
    // A round takes what was there when it began and the next takes what the
    // last one made. So the order inside a round is the order it always was,
    // every new declaration is reached, and nothing is typed -- or reported --
    // twice
    while (done < count) {
        scope_of.clear();

        for (u32 scope = 1; scope < table->get_scope_count(); scope++) {
            u32 owner = table->get_scope(scope)->owner;

            if (owner != 0) {
                scope_of[owner] = scope;
            }
        }

        for (u32 scope = 1; scope < table->get_scope_count(); scope++) {
            for (u32 symbol = table->get_scope(scope)->symbols; symbol != 0;
                 symbol = table->get_symbol(symbol)->sibling_or_next) {
                for (u32 candidate = table->get_symbol(symbol)->candidates;
                     candidate != 0;
                     candidate =
                         table->get_candidate(candidate)->next_candidate) {
                    if (candidate <= done || candidate >= count) {
                        continue;
                    }

                    // a declaration made during the second pass has never been
                    // through the first, and what the second pass reads is
                    // what the first one wrote. So it gets both, in order
                    if (given && candidate > (collected.count(index) > 0
                                                  ? collected[index]
                                                  : 0)) {
                        table->set_candidate_type(
                            candidate, type_of(candidate, scope, false));
                        table->set_candidate_super(candidate,
                                                   super_of(candidate, scope));
                    }

                    table->set_candidate_type(candidate,
                                              type_of(candidate, scope, given));

                    if (!given) {
                        table->set_candidate_super(candidate,
                                                   super_of(candidate, scope));

                        // once per candidate, which the mark above is what
                        // guarantees: the loop re-enters a module that grew
                        // and a diagnostic must not come out twice
                        require_default_construction(candidate);
                    }

                    worked = true;
                }
            }
        }

        done = count;
        count = table->get_candidate_count();
    }

    mark[index] = done;

    return worked;
}

u32 TypeCollector::type_of(u32 candidate, u32 scope, bool given) {
    SymbolTable* table = module->get_symbols();
    Candidate* found = table->get_candidate(candidate);
    AstQuery query;

    query.set_module(module);

    switch ((SymbolKind) found->kind) {
    // its parameters and its own generic parameters are in the scope it
    // opened, not in the one it sits in
    case SYMBOL_FUNCTION:
        return signature_of(found->ast_node,
                            scope_of.count(found->ast_node) > 0
                                ? scope_of[found->ast_node]
                                : scope);

    // a declaration that defines a type *is* one. The generic arguments are
    // not part of it: 'Node' is the declaration and 'Node<i32>' is a use of it,
    // built where it is written
    case SYMBOL_CLASS:
    case SYMBOL_STRUCT:
    case SYMBOL_ENUM:
    case SYMBOL_UNION:
        return module->get_types()->named(index, candidate,
                                          std::vector<u32>());

    // An instantiation bound this one when it cloned the declaration, and
    // that binding is the whole of what makes the clone concrete -- so it
    // survives this walk rather than being overwritten by the parameter's own
    // TYPE_GENERIC. For an unbound parameter the branch below wrote that
    // TYPE_GENERIC on an earlier round and reading it back says the same thing
    case SYMBOL_GENERIC:
        if (found->type != INVALID_TYPE) {
            return found->type;
        }

        return module->get_types()->generic(index, candidate);

    // a field, a parameter or a variable is whatever it was written as. A
    // 'let' with no type waits for inference, which does not exist
    default:
        break;
    }

    if (!given) {
        return builder.build(index, scope,
                             query.get_written_type(found->ast_node));
    }

    return written_or_inferred(found->ast_node, scope, found->type);
}

u32 TypeCollector::super_of(u32 candidate, u32 scope) {
    SymbolTable* table = module->get_symbols();
    Candidate* found = table->get_candidate(candidate);
    AstQuery query;
    u32 written;

    switch ((SymbolKind) found->kind) {
    case SYMBOL_CLASS:
    case SYMBOL_STRUCT:
    case SYMBOL_UNION:
        break;

    default:
        return INVALID_TYPE;
    }

    query.set_module(module);
    written = query.get_super_type(found->ast_node);

    // built in the scope the declaration opened, so 'class Box<T>(Holder<T>)'
    // has its own generic parameter in view
    return builder.build(index, scope_of.count(found->ast_node) > 0
                                    ? scope_of[found->ast_node]
                                    : scope,
                         written);
}

u32 TypeCollector::written_or_inferred(u32 node, u32 scope, u32 written) {
    AstQuery query;
    u32 expression;

    query.set_module(module);
    expression = query.get_binding_expression(node);

    if (expression == 0) {
        return written;
    }

    // the written type is handed down, so a literal takes it rather than being
    // converted into it. With nothing written the expression decides, which is
    // the whole of inference for a binding
    u32 given = typer.type_of(index, scope, expression, written);

    if (written == INVALID_TYPE) {
        return given;
    }

    // a literal already took the written type or complained about the value,
    // so what is left here is two real types, and record 0018's list is what
    // says whether one may be given to the other -- the same list a call
    // asks, which is the point of it living in one place
    if (given != INVALID_TYPE && !coercion.fits(index, given, written)) {
        Token& token = module->get_tokens()->get_token(
            module->get_ast()->get_node(expression)->get_token());

        module->get_logger()->error(
            token.get_offset(), token.get_length(),
            "expected " + typer.name_of(written) + ", found " +
                typer.name_of(given));
    }

    return written;
}

u32 TypeCollector::signature_of(u32 node, u32 scope) {
    AstQuery query;
    std::vector<u32> parameters;
    u32 result;

    query.set_module(module);

    for (u32 param : query.get_params(node)) {
        parameters.push_back(
            builder.build(index, scope, query.get_written_type(param)));
    }

    u32 written = query.get_written_type(node);

    result = builder.build(index, scope, written);

    // The grammar makes a return type mandatory, so a missing one is a tree
    // that did not parse and cannot reach here. A written one that names
    // nothing is a different thing and poisons the signature, the way record
    // 0016 has any unbuildable part poison what contains it
    if (written == 0) {
        result = module->get_types()->builtin(BUILTIN_VOID);
    } else if (result == INVALID_TYPE) {
        return INVALID_TYPE;
    }

    for (u32 parameter : parameters) {
        if (parameter == INVALID_TYPE) {
            return INVALID_TYPE;
        }
    }

    return module->get_types()->function(parameters, result);
}

void TypeCollector::require_default_construction(u32 candidate) {
    SymbolTable* table = module->get_symbols();
    Candidate* found = table->get_candidate(candidate);
    AstQuery query;
    u32 wanted;
    std::string where;

    query.set_module(module);

    switch ((SymbolKind) found->kind) {
    // the base runs before the derived's own body, so a class that cannot be
    // built with nothing cannot be derived from either
    case SYMBOL_CLASS:
    case SYMBOL_STRUCT:
    case SYMBOL_UNION:
        wanted = found->super;
        // and it names why this one cannot be fixed where the others can:
        // record 0026 leaves 'super(...)' undecided, so there is nowhere to
        // write a base's arguments even when the author knows them
        where = "a base cannot be given one";
        break;

    // a field held by value and a binding with no expression both come into
    // being running the class's 'init' with nothing. One that was given a
    // value does not
    case SYMBOL_FIELD:
    case SYMBOL_VARIABLE:
        if (query.get_binding_expression(found->ast_node) != 0) {
            return;
        }

        wanted = found->type;
        where = "none is written here";
        break;

    // a parameter is passed and never built, and a function is not a value
    default:
        return;
    }

    if (builds_with_nothing(wanted)) {
        return;
    }

    Type* entry = module->get_types()->get_type(wanted);
    Module* holder = compilation->get_module(entry->module);
    AstQuery theirs;

    theirs.set_module(holder);

    report(name_node_of(found->ast_node),
           "every 'init' of "
               + theirs.get_declaration_name(
                     holder->get_symbols()->get_candidate(entry->subject)
                         ->ast_node)
               + " takes an argument, and " + where);
}

bool TypeCollector::builds_with_nothing(u32 type) {
    if (type == INVALID_TYPE) {
        return true;
    }

    Type* entry = module->get_types()->get_type(type);

    // only a class is built; a pointer, a builtin and an array of them are
    // whatever the memory held, which is record 0026's rule 3
    if (entry->kind != TYPE_NAMED) {
        return true;
    }

    Module* holder = compilation->get_module(entry->module);
    u32 declaration =
        holder->get_symbols()->get_candidate(entry->subject)->ast_node;
    AstQuery query;
    bool has_init = false;

    query.set_module(holder);

    for (u32 member : query.get_members(declaration)) {
        if (query.get_declaration_name(member) != "init") {
            continue;
        }

        has_init = true;

        // record 0012 makes arity a range, so an 'init' whose every parameter
        // has a default answers to no arguments as well -- which is how a
        // class that wants to be derived from is written today
        bool needs_nothing = true;

        for (u32 parameter : query.get_params(member)) {
            if (query.get_binding_expression(parameter) == 0) {
                needs_nothing = false;
            }
        }

        if (needs_nothing) {
            return true;
        }
    }

    // no 'init' at all is an aggregate, and C++ builds it for nothing
    return !has_init;
}

u32 TypeCollector::name_node_of(u32 declaration) {
    Ast* ast = module->get_ast();
    u32 wrapper = ast->get_node(declaration)->get_children();

    if (wrapper == 0
        || ast->get_node(wrapper)->get_kind() != AST_BINDING_NAME) {
        return declaration;
    }

    u32 identifier = ast->get_node(wrapper)->get_children();

    return identifier == 0 ? declaration : identifier;
}

void TypeCollector::report(u32 node, const std::string& message) {
    Token& token = module->get_tokens()->get_token(
        module->get_ast()->get_node(node)->get_token());

    module->get_logger()->error(token.get_offset(), token.get_length(),
                                message);
}
