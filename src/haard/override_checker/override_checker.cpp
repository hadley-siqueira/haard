#include <haard/override_checker/override_checker.h>
#include <haard/string_table/string_table.h>

using namespace haard;

OverrideChecker::OverrideChecker() {
    compilation = nullptr;
    module = nullptr;
    index = 0;
}

void OverrideChecker::set_compilation(Compilation* compilation) {
    this->compilation = compilation;

    builder.set_compilation(compilation);
    typer.set_compilation(compilation);
}

void OverrideChecker::check(u32 index) {
    this->index = index;
    module = compilation->get_module(index);

    query.set_module(module);
    typer.set_module(index);

    SymbolTable* table = module->get_symbols();

    // straight down the candidate vector. Every declaration is in it exactly
    // once, which is what this phase wants -- unlike a walk of scopes, where a
    // method would be reached through the scope its class opened and the
    // class through the one above it
    for (u32 candidate = 1; candidate < table->get_candidate_count();
         candidate++) {
        switch ((SymbolKind) table->get_candidate(candidate)->kind) {
        case SYMBOL_CLASS:
        case SYMBOL_STRUCT:
        case SYMBOL_UNION:
            check_class(candidate);
            break;

        default:
            break;
        }
    }
}

void OverrideChecker::check_class(u32 candidate) {
    SymbolTable* table = module->get_symbols();
    Candidate* found = table->get_candidate(candidate);
    u32 super = found->super;

    // a class with no base overrides nothing, and one whose base could not be
    // built was reported where the base was written
    if (super == INVALID_TYPE) {
        return;
    }

    u32 body = table->scope_owned_by(found->ast_node);

    if (body == 0) {
        return;
    }

    for (u32 symbol = table->get_scope(body)->symbols; symbol != 0;
         symbol = table->get_symbol(symbol)->sibling_or_next) {
        for (u32 method = table->get_symbol(symbol)->candidates; method != 0;
             method = table->get_candidate(method)->next_candidate) {
            // a field is not an override and cannot be one. Two fields of one
            // name give the object both, and record 0020 makes a bare name in
            // a method of the base still mean the base's -- so the same name
            // written in two methods of one object reads two different pieces
            // of memory, and nothing in the source says which. There is no
            // rule that makes that useful, so it is an error and not shadowing
            if (table->get_candidate(method)->kind == SYMBOL_FIELD) {
                Candidacy owner = declared_above(method, super);

                if (owner.candidate != 0) {
                    report(name_node_of(table->get_candidate(method)->ast_node),
                           name_of_declaration(owner)
                               + " already declares this field");
                }

                continue;
            }

            if (table->get_candidate(method)->kind != SYMBOL_FUNCTION
                || table->get_candidate(method)->type == INVALID_TYPE) {
                continue;
            }

            Candidacy above = overridden_by(method, super);

            if (above.candidate == 0) {
                continue;
            }

            u32 mine = result_of(index, method);
            u32 theirs = result_of(above.module, above.candidate);

            if (may_give_back(mine, theirs)) {
                continue;
            }

            report(name_node_of(table->get_candidate(method)->ast_node),
                   "this overrides " + qualified(above.module, above.candidate)
                       + ", which gives back " + name_of(theirs)
                       + ", and gives back " + name_of(mine));
        }
    }
}

std::vector<Candidacy> OverrideChecker::bases_of(u32 super) {
    std::vector<Candidacy> chain;
    u32 above = super;

    while (above != INVALID_TYPE) {
        Type* entry = module->get_types()->get_type(above);

        if (entry->kind != TYPE_NAMED) {
            break;
        }

        Candidacy one = Candidacy{entry->module, entry->subject};
        bool seen = false;

        // a cycle in the bases, which nothing rejects yet. Stopping keeps the
        // walk finite and is not a diagnostic
        for (const Candidacy& already : chain) {
            if (already.module == one.module
                && already.candidate == one.candidate) {
                seen = true;
                break;
            }
        }

        if (seen) {
            break;
        }

        chain.push_back(one);

        SymbolTable* theirs =
            compilation->get_module(one.module)->get_symbols();

        above = builder.translate(index, one.module,
                                  theirs->get_candidate(one.candidate)->super);
    }

    return chain;
}

u32 OverrideChecker::symbol_in(const Candidacy& base, u32 hash,
                               const std::string& name) {
    Module* holder = compilation->get_module(base.module);
    SymbolTable* theirs = holder->get_symbols();

    // the name means nothing in the base's module until it is interned
    // there -- record 0013's rule for a lookup that crosses an import
    u32 interned = holder->get_strings()->find(hash, name);
    u32 body =
        theirs->scope_owned_by(theirs->get_candidate(base.candidate)->ast_node);

    if (interned == INVALID_STRING || body == 0) {
        return 0;
    }

    return theirs->find(body, interned);
}

Candidacy OverrideChecker::overridden_by(u32 candidate, u32 super) {
    SymbolTable* table = module->get_symbols();
    std::string name = query.get_declaration_name(
        table->get_candidate(candidate)->ast_node);
    std::vector<u32> wanted = parameters_of(index, candidate);
    u32 hash = hash_name(name);

    for (const Candidacy& base : bases_of(super)) {
        SymbolTable* theirs =
            compilation->get_module(base.module)->get_symbols();
        u32 symbol = symbol_in(base, hash, name);

        for (u32 one = symbol == 0 ? 0 : theirs->get_symbol(symbol)->candidates;
             one != 0; one = theirs->get_candidate(one)->next_candidate) {
            if (theirs->get_candidate(one)->kind != SYMBOL_FUNCTION
                || theirs->get_candidate(one)->type == INVALID_TYPE) {
                continue;
            }

            // record 0020: the same parameters and nothing else. The first one
            // found going up is the one that matters -- a class in between
            // would have overridden it first, and this method overrides that
            if (parameters_of(base.module, one) == wanted) {
                return Candidacy{base.module, one};
            }
        }
    }

    return Candidacy{0, 0};
}

Candidacy OverrideChecker::declared_above(u32 candidate, u32 super) {
    SymbolTable* table = module->get_symbols();
    std::string name = query.get_declaration_name(
        table->get_candidate(candidate)->ast_node);
    u32 hash = hash_name(name);

    for (const Candidacy& base : bases_of(super)) {
        SymbolTable* theirs =
            compilation->get_module(base.module)->get_symbols();
        u32 symbol = symbol_in(base, hash, name);

        for (u32 one = symbol == 0 ? 0 : theirs->get_symbol(symbol)->candidates;
             one != 0; one = theirs->get_candidate(one)->next_candidate) {
            // a method of that name is not what this asks about: a field and a
            // method may share a name the way any two kinds may, and record
            // 0012 puts both in the candidate list of one symbol
            if (theirs->get_candidate(one)->kind == SYMBOL_FIELD) {
                // the class and not the field, which is what the message
                // names -- the span already points at the field
                return base;
            }
        }
    }

    return Candidacy{0, 0};
}

std::string OverrideChecker::name_of_declaration(const Candidacy& one) {
    Module* holder = compilation->get_module(one.module);
    AstQuery theirs;

    theirs.set_module(holder);

    return theirs.get_declaration_name(
        holder->get_symbols()->get_candidate(one.candidate)->ast_node);
}

bool OverrideChecker::may_give_back(u32 derived, u32 base) {
    if (derived == base) {
        return true;
    }

    // one of them could not be built, and that was reported where it was
    // written. A second complaint about the same mistake helps nobody
    if (derived == INVALID_TYPE || base == INVALID_TYPE) {
        return true;
    }

    TypeTable* types = module->get_types();
    Type* below = types->get_type(derived);
    Type* above = types->get_type(base);

    // Covariance is for a pointer and for a reference and never for a value,
    // and the reason is size: somebody calling through the base reserved room
    // for the base, and the derived one does not fit. Two pointers are the
    // same size whatever they point at, which is what makes those safe
    if (below->kind != above->kind
        || (below->kind != TYPE_POINTER && below->kind != TYPE_REFERENCE)) {
        return false;
    }

    // strictly below: the same class is the equality above, and anything else
    // has to be a class this one derives from
    return distance(types->get_argument(below->first_argument),
                    types->get_argument(above->first_argument)) > 0;
}

int OverrideChecker::distance(u32 from, u32 to) {
    TypeTable* types = module->get_types();
    int steps = 0;

    // single inheritance makes this a walk up a chain and never a search
    while (from != INVALID_TYPE) {
        if (from == to) {
            return steps;
        }

        Type* entry = types->get_type(from);

        if (entry->kind != TYPE_NAMED) {
            return -1;
        }

        Module* owner = compilation->get_module(entry->module);
        u32 base = owner->get_symbols()->get_candidate(entry->subject)->super;

        from = builder.translate(index, entry->module, base);
        steps++;
    }

    return -1;
}

std::vector<u32> OverrideChecker::parameters_of(u32 owner, u32 candidate) {
    Module* holder = compilation->get_module(owner);
    Candidate* found = holder->get_symbols()->get_candidate(candidate);
    std::vector<u32> written =
        holder->get_types()->get_arguments(found->type);

    // the return is the last one, per record 0016, and record 0012 keeps it
    // out of what tells two overloads apart -- so it is out of what makes one
    // an override too
    written.pop_back();

    for (u32& one : written) {
        one = builder.translate(index, owner, one);
    }

    return written;
}

u32 OverrideChecker::result_of(u32 owner, u32 candidate) {
    Module* holder = compilation->get_module(owner);
    Candidate* found = holder->get_symbols()->get_candidate(candidate);

    return builder.translate(
        index, owner, holder->get_types()->get_arguments(found->type).back());
}

std::string OverrideChecker::qualified(u32 owner, u32 candidate) {
    Module* holder = compilation->get_module(owner);
    SymbolTable* table = holder->get_symbols();
    Candidate* found = table->get_candidate(candidate);
    AstQuery theirs;

    theirs.set_module(holder);

    // the scope the method opened, out one step to the class body, and the
    // node that opened that is the class
    u32 inside = table->scope_owned_by(found->ast_node);
    u32 around = inside == 0 ? 0 : table->get_scope(inside)->parent;
    u32 owner_node = around == 0 ? 0 : table->get_scope(around)->owner;

    return (owner_node == 0 ? ""
                            : theirs.get_declaration_name(owner_node) + ".")
         + theirs.get_declaration_name(found->ast_node);
}

std::string OverrideChecker::name_of(u32 type) {
    return typer.name_of(type);
}

u32 OverrideChecker::name_node_of(u32 declaration) {
    Ast* ast = module->get_ast();
    u32 wrapper = ast->get_node(declaration)->get_children();

    if (wrapper == 0
        || ast->get_node(wrapper)->get_kind() != AST_BINDING_NAME) {
        return declaration;
    }

    u32 identifier = ast->get_node(wrapper)->get_children();

    return identifier == 0 ? declaration : identifier;
}

void OverrideChecker::report(u32 node, const std::string& message) {
    Token& token = module->get_tokens()->get_token(
        module->get_ast()->get_node(node)->get_token());

    module->get_logger()->error(token.get_offset(), token.get_length(),
                                message);
}
