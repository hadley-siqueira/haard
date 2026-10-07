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
            check_layout(candidate);
            check_class(candidate);
            break;

        default:
            break;
        }
    }

    check_duplicates();
}

// One scope may hold one declaration of a name, except for a function: record
// 0012 makes several functions of one name an overload set, told apart by
// their parameters. So two of anything else is an error, and so are two
// functions whose parameters are the same -- no call could choose between
// them. Until 2026-09-24 both went through: two classes 'Box', two fields 'x'
// and two 'f(i32)' compiled and ran, and two 'let x' reached the emitter.
//
// Here and not earlier because telling two functions apart takes their
// signatures, and this is the phase that reads a signature and reports about
// a declaration. Each repeat is reported where it is written, once
void OverrideChecker::check_duplicates() {
    SymbolTable* table = module->get_symbols();

    for (u32 scope = 1; scope < table->get_scope_count(); scope++) {
        for (u32 symbol = table->get_scope(scope)->symbols; symbol != 0;
             symbol = table->get_symbol(symbol)->sibling_or_next) {
            std::vector<u32> seen;

            for (u32 candidate = table->get_symbol(symbol)->candidates;
                 candidate != 0;
                 candidate = table->get_candidate(candidate)->next_candidate) {
                // record 0072: a pattern's name that is a variant declares
                // nothing, however many times the pattern writes it
                if (module->is_unnamed(candidate)) {
                    continue;
                }

                for (u32 earlier : seen) {
                    if (!same_declaration(earlier, candidate)) {
                        continue;
                    }

                    Candidate* one = table->get_candidate(candidate);
                    std::string name = query.get_declaration_name(
                        one->ast_node);

                    if (one->kind == SYMBOL_FUNCTION) {
                        report(name_node_of(one->ast_node),
                               "'" + name + "' is already declared here with "
                               "the same parameters");
                    } else {
                        report(name_node_of(one->ast_node),
                               "'" + name + "' is already declared in this "
                               "scope");
                    }

                    break;
                }

                seen.push_back(candidate);
            }
        }
    }
}

// whether two declarations of one name in one scope cannot both stand
bool OverrideChecker::same_declaration(u32 first, u32 second) {
    SymbolTable* table = module->get_symbols();
    Candidate* one = table->get_candidate(first);
    Candidate* other = table->get_candidate(second);

    if (one->kind != SYMBOL_FUNCTION || other->kind != SYMBOL_FUNCTION) {
        return true;
    }

    // a signature that did not build was reported where it was written
    if (one->type == INVALID_TYPE || other->type == INVALID_TYPE
        || module->get_types()->get_type(one->type)->kind != TYPE_FUNCTION
        || module->get_types()->get_type(other->type)->kind != TYPE_FUNCTION) {
        return false;
    }

    return parameters_of(index, first) == parameters_of(index, second);
}

void OverrideChecker::check_class(u32 candidate) {
    SymbolTable* table = module->get_symbols();
    Candidate* found = table->get_candidate(candidate);
    u32 super = found->super;

    // a class with no base overrides nothing, and one whose base could not be
    // built was reported where the base was written. It is not a reason to
    // stop, though: agenda 5.6's question is about a method on its own and is
    // asked of every class, base or no base
    bool derived = super != INVALID_TYPE;

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
                if (!derived) {
                    continue;
                }

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

            if (check_construction(method)) {
                continue;
            }

            if (!derived) {
                continue;
            }

            Candidacy holder = Candidacy{0, 0};
            Candidacy above = overridden_by(method, super, holder);

            if (above.candidate == 0) {
                continue;
            }

            // Record 0065, Hadley 2026-09-29: a struct has no vtable, so its
            // methods are not virtual, and one written again below it would
            // be a second method chosen by the type the caller happens to
            // hold -- which record 0020 says the same parameters never are.
            // A constructor and a destructor are not dispatched at all
            std::string name = query.get_declaration_name(
                table->get_candidate(method)->ast_node);

            if (compilation->get_module(holder.module)->get_symbols()
                        ->get_candidate(holder.candidate)->kind
                    == SYMBOL_STRUCT
                && name != "init" && name != "destroy") {
                report(name_node_of(table->get_candidate(method)->ast_node),
                       qualified(above.module, above.candidate)
                           + " is a method of a struct, which is not virtual "
                             "and cannot be overridden");
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

// Record 0047: the compiler refuses when it cannot decide, and each rule for a
// union is a question nothing in it answers -- which field to make, copy or
// end, which one a value written on two of them is in. C++ answers none of them
// either: it deletes the union's constructor, destructor and copy, and says so
// about a line nobody wrote.
//
// A struct's rule is Hadley's, 2026-09-29: a struct can always model plain
// data, so it never has a vtable -- and a base class would give it one
void OverrideChecker::check_layout(u32 candidate) {
    SymbolTable* table = module->get_symbols();
    Candidate* found = table->get_candidate(candidate);
    SymbolKind base = SYMBOL_NONE;

    if (found->super != INVALID_TYPE) {
        Type* entry = module->get_types()->get_type(found->super);

        if (entry->kind == TYPE_NAMED) {
            base = (SymbolKind) compilation->get_module(entry->module)
                       ->get_symbols()->get_candidate(entry->subject)->kind;
        }
    }

    if (found->kind == SYMBOL_UNION && found->super != INVALID_TYPE) {
        report(name_node_of(found->ast_node),
               "a union cannot derive from anything");
    } else if (base == SYMBOL_UNION) {
        report(name_node_of(found->ast_node),
               "nothing can derive from a union");
    } else if (found->kind == SYMBOL_STRUCT && base == SYMBOL_CLASS) {
        report(name_node_of(found->ast_node),
               "a struct cannot derive from a class: a struct has no vtable, "
               "and a class has one");
    }

    if (found->kind != SYMBOL_UNION) {
        return;
    }

    bool given = false;

    for (u32 member : query.get_members(found->ast_node)) {
        if (module->get_ast()->get_node(member)->get_kind() != AST_FIELD) {
            continue;
        }

        u32 field = table->candidate_of(member);
        u32 type = field == 0 ? INVALID_TYPE : table->get_candidate(field)->type;

        if (type != INVALID_TYPE && !has_no_lifetime(index, type)) {
            report(name_node_of(member),
                   "a union cannot hold " + name_of(type)
                       + ": nothing says which field is alive, so nothing "
                         "could make, copy or end it");
        }

        if (query.get_binding_expression(member) == 0) {
            continue;
        }

        // the first field given a value is the one the union is made
        // holding; a second would be a second field alive at once
        if (given) {
            report(name_node_of(member),
                   "only one field of a union may be given a value: it is "
                   "the one alive when the union is made");
        }

        given = true;
    }
}

bool OverrideChecker::has_no_lifetime(u32 owner, u32 type) {
    TypeTable* types = compilation->get_module(owner)->get_types();
    Type* entry = types->get_type(type);

    switch ((TypeKind) entry->kind) {
    case TYPE_BUILTIN:
    case TYPE_POINTER:
    // a generic's own fields: asked again of each clone, where 'T' is a type
    case TYPE_GENERIC:
        return true;

    // 'T[]' is sugar for Array<T> (record 0036), and 'T[4]' is four of T
    case TYPE_ARRAY:
        return entry->subject != NO_LENGTH
               && has_no_lifetime(owner,
                                  types->get_argument(entry->first_argument));

    case TYPE_NAMED:
        break;

    default:
        return false;
    }

    Module* holder = compilation->get_module(entry->module);
    SymbolTable* theirs = holder->get_symbols();
    Candidate* named = theirs->get_candidate(entry->subject);
    AstQuery read;

    read.set_module(holder);

    // a class has a vtable, and so a constructor that sets it
    if (named->kind == SYMBOL_CLASS) {
        return false;
    }

    // an enum that carries nothing is an integer; one that carries something
    // is a struct with a union in it, and whether that has a lifetime depends
    // on what it carries -- not asked yet
    if (named->kind == SYMBOL_ENUM) {
        for (u32 member : read.get_members(named->ast_node)) {
            if (holder->get_ast()->get_node(member)->get_kind() == AST_FIELD
                && read.get_written_type(member) != 0) {
                return false;
            }
        }

        return true;
    }

    std::pair<u32, u32> key(entry->module, entry->subject);

    if (walking.count(key) > 0) {
        return true;
    }

    walking.insert(key);

    bool plain = is_plain(entry->module, entry->subject);

    walking.erase(key);

    return plain;
}

// Record 0065: a struct -- or a union inside a union -- is plain data when
// nothing has to run for it. An 'init', a 'destroy' or a field given a value is
// something that runs; so is a base or a field that has one
bool OverrideChecker::is_plain(u32 owner, u32 candidate) {
    Module* holder = compilation->get_module(owner);
    SymbolTable* theirs = holder->get_symbols();
    Candidate* named = theirs->get_candidate(candidate);
    AstQuery read;

    read.set_module(holder);

    if (named->super != INVALID_TYPE
        && !has_no_lifetime(owner, named->super)) {
        return false;
    }

    for (u32 member : read.get_members(named->ast_node)) {
        AstNodeKind kind = holder->get_ast()->get_node(member)->get_kind();

        if (kind == AST_FUNCTION
            && (read.get_declaration_name(member) == "init"
                || read.get_declaration_name(member) == "destroy")) {
            return false;
        }

        if (kind != AST_FIELD) {
            continue;
        }

        if (read.get_binding_expression(member) != 0) {
            return false;
        }

        u32 field = theirs->candidate_of(member);

        if (field != 0 && theirs->get_candidate(field)->type != INVALID_TYPE
            && !has_no_lifetime(owner, theirs->get_candidate(field)->type)) {
            return false;
        }
    }

    return true;
}

// Agenda 5.6. Record 0026 makes 'init' and 'destroy' ordinary methods, and
// that is what left this open: an ordinary method may give something back, and
// these two are run by something that has nowhere to put it. The constructor
// the emitter writes calls 'init' and drops the answer; 'destroy' is called
// from a destructor, which in C++ cannot even be asked. So a written return
// type is not a thing the compiler can honour and it is said so about.
//
// Only the return. What they take is what tells two 'init's apart -- record
// 0038 makes the copy constructor an 'init' of one parameter -- and 'destroy'
// taking arguments is a method nobody can call, which is a different question
// nobody has asked
bool OverrideChecker::check_construction(u32 candidate) {
    SymbolTable* table = module->get_symbols();
    std::string name = query.get_declaration_name(
        table->get_candidate(candidate)->ast_node);

    if (name != "init" && name != "destroy") {
        return false;
    }

    u32 result = result_of(index, candidate);

    // a signature with a part that would not build is poisoned whole -- record
    // 0016 -- and was reported where the part was written
    if (result == INVALID_TYPE
        || result == module->get_types()->builtin(BUILTIN_VOID)) {
        return false;
    }

    report(name_node_of(table->get_candidate(candidate)->ast_node),
           std::string(name == "init" ? "an '" : "a '") + name
               + "' gives back nothing, and this gives back "
               + name_of(result));

    return true;
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

Candidacy OverrideChecker::overridden_by(u32 candidate, u32 super,
                                        Candidacy& holder) {
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
                holder = base;
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
