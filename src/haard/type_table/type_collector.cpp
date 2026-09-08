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

    lowerer.set_compilation(compilation);

    // both builders that can reach an instantiation are told who to catch up,
    // and so is the one the foreach lowering builds with: 'for x in [1, 2,
    // 3]' asks an Array cloned mid-sweep for a cursor
    builder.set_collector(this);
    typer.set_collector(this);
    lowerer.set_collector(this);
}

bool TypeCollector::collect(u32 index) {
    return walk(index, false);
}

bool TypeCollector::infer(u32 index) {
    return walk(index, true);
}

void TypeCollector::catch_up(u32 module_index) {
    // already inside a walk of it, and that walk's own loop re-reads the
    // count every round -- so it will reach what was just added by itself
    if (walking.count(module_index) > 0) {
        return;
    }

    // the walk keeps its subject in members, and this one is nested inside
    // another, so what the outer one was looking at is put back afterwards
    u32 held_index = index;
    Module* held_module = module;
    std::map<u32, u32> held_scope_of = scope_of;

    // the **written** pass, and not the inferred one. What a caller needs of
    // a fresh clone is its methods' signatures, and a signature is written --
    // 'xs.length()' resolves against parameters and a return type. Its local
    // bindings can wait for the ordinary sweep, and must: typing them now
    // would mark them done before what they depend on exists, and nothing
    // retries a candidate the mark has passed
    walk(module_index, false);

    index = held_index;
    module = held_module;
    scope_of = held_scope_of;
}

bool TypeCollector::walk(u32 index, bool given) {
    this->index = index;
    module = compilation->get_module(index);

    SymbolTable* table = module->get_symbols();
    std::map<u32, u32>& mark = given ? inferred : collected;
    u32 done = mark.count(index) > 0 ? mark[index] : 0;
    u32 count = table->get_candidate_count();
    bool worked = false;

    walking.insert(index);

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

                    // Record 0002: the clone is what a use names, and the
                    // clone is typed on its own. What is written inside the
                    // generic itself is not a program yet.
                    //
                    // The **parameters** are not part of that: 'T' is what
                    // makes the declaration generic, it is what the
                    // instantiator binds, and it is what says a declaration
                    // is still unbound


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

    walking.erase(index);

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

    // A **capture** of a 'case': its candidate points at the switch and its
    // scope is owned by the case, which is the pair the answer needs. The
    // switch says which enum, and the case's pattern says which variant and
    // at which position -- 'case Click(x, y)' binds x to the first thing
    // Click carries and y to the second
    if ((SymbolKind) found->kind == SYMBOL_VARIABLE
        && module->get_ast()->get_node(found->ast_node)->get_kind()
               == AST_SWITCH) {
        if (!given) {
            return INVALID_TYPE;
        }

        return capture_of(found->ast_node, table->get_scope(scope)->owner,
                          candidate, scope);
    }

    // Record 0040. A loop variable's candidate points at the **loop**, since
    // from the name alone there is no way back to the sequence it comes out
    // of -- so this is where a foreach is taken apart, and what the variable
    // is, is what the loop it became gives it.
    //
    // Here and not in a phase of its own because of the order: the sequence
    // was typed a moment ago, in the scope that encloses this one, and the
    // body's own bindings are typed in a moment, after this. A pass over the
    // whole tree afterwards would be too late for both
    if (module->get_ast()->get_node(found->ast_node)->get_kind()
        == AST_FOR_EACH) {
        u32 loop = found->ast_node;

        // the first pass gives a declaration the type it **wrote**, and a
        // loop variable writes none. What is walked is a 'let' with no type
        // as often as not, and that is this pass's answer and not the first
        // one's
        if (!given) {
            return INVALID_TYPE;
        }

        u32 expected = INVALID_TYPE;
        u32 binding = lowerer.lower(index, scope, loop, expected);

        // left standing, and reported. Nothing else in the tree changed
        if (binding == 0) {
            return INVALID_TYPE;
        }

        // and from here it is an ordinary binding written inside the body:
        // the emitter reads a declaration where it declares, and what it was
        // given is inferred the way every other one is. 'found' is not read
        // again -- the lowering declares locals of its own, and a candidate
        // list that grew has moved
        table->set_candidate_node(candidate, binding);

        return written_or_inferred(binding, scope, expected);
    }

    // What a variant of an enum IS, and it is two things.
    //
    // One that carries nothing **is one of the enum**: 'Colour.red' is a
    // Colour, which is what makes the payload-free enum the degenerate case
    // of the tagged union rather than a second feature.
    //
    // One that carries something is a **constructor**: 'Click : (i32, i32)'
    // means Action is built out of two i32s, so its type is a signature and
    // 'Action.Click(10, 20)' is an ordinary call that gives back an Action.
    // A tuple payload is **flattened** -- two parameters and not one tuple --
    // which is the shape Hadley's 'Click(10, 20)' asks for and the one Rust
    // gives a tuple variant
    if ((SymbolKind) found->kind == SYMBOL_VARIANT) {
        u32 owner = table->get_scope(scope)->owner;
        u32 whole = owner == 0 ? INVALID_TYPE
                               : module->get_types()->named(
                                     index, table->candidate_of(owner),
                                     std::vector<u32>());
        u32 written = query.get_written_type(found->ast_node);

        if (written == 0 || whole == INVALID_TYPE) {
            return whole;
        }

        std::vector<u32> carried = payload_of(written, scope);

        for (u32 one : carried) {
            if (one == INVALID_TYPE) {
                return INVALID_TYPE;
            }

            // A variant that carries the enum it belongs to **by value** has
            // no size: the union would hold something as big as itself.
            // Hadley, 2026-09-08: write a pointer, and nothing allocates
            // behind the author's back the way Rust's Box and Swift's
            // 'indirect' do
            if (one == whole) {
                // in the written pass and not in both, since the mark is per
                // pass and a variant goes through each -- one mistake is one
                // error
                if (!given) {
                    report(name_node_of(found->ast_node),
                           "a variant cannot carry the enum it belongs to by "
                           "value: write a pointer");
                }

                return INVALID_TYPE;
            }

            // A class that **cannot be copied** cannot be carried either:
            // the union's copy asks each variant to copy what it holds, so a
            // class that owns something and says nothing about being copied
            // (record 0031) would leave the enum with no copy at all
            if (!coercion.may_be_copied(index, one)) {
                if (!given) {
                    report(name_node_of(found->ast_node),
                           typer.name_of(one)
                               + " cannot be copied, and a variant that "
                                 "carries one would have to be");
                }

                return INVALID_TYPE;
            }
        }

        return module->get_types()->function(carried, whole);
    }

    if (!given) {
        return builder.build(index, scope,
                             query.get_written_type(found->ast_node));
    }

    return written_or_inferred(found->ast_node, scope, found->type);
}

// What a variant carries, flattened: a tuple payload is its elements and
// anything else is itself. Written out rather than kept as a tuple because a
// variant is a constructor and 'Click(10, 20)' passes two arguments -- and
// because a tuple is not a type this back end can write
// What a capture is: the thing its variant carries at the position the name
// was written in. 'case Click(x, y)' binds x to the first thing Click carries
// and y to the second, which is the constructor's signature read the other
// way round.
//
// Two halves and they come from two places: the **switch** says which enum,
// because its subject is a value of one, and the **case** says which variant
// and which position. That is why a capture's candidate points at the one and
// its scope is owned by the other
u32 TypeCollector::capture_of(u32 switch_node, u32 one_case, u32 candidate,
                              u32 scope) {
    Ast* ast = module->get_ast();
    AstQuery mine;
    u32 subject = ast->get_node(switch_node)->get_children();

    if (subject == 0 || one_case == 0) {
        return INVALID_TYPE;
    }

    mine.set_module(module);

    u32 given = typer.type_of(index, scope, subject, INVALID_TYPE);

    if (given == INVALID_TYPE) {
        return INVALID_TYPE;
    }

    TypeTable* types = module->get_types();
    Type* entry = types->get_type(types->value_of(given));

    if (entry->kind != TYPE_NAMED) {
        return INVALID_TYPE;
    }

    Module* holder = compilation->get_module(entry->module);
    SymbolTable* theirs = holder->get_symbols();

    if ((SymbolKind) theirs->get_candidate(entry->subject)->kind
        != SYMBOL_ENUM) {
        return INVALID_TYPE;
    }

    // which of the names in the pattern this candidate is
    std::string wanted = name_of_candidate(scope, candidate);
    std::vector<u32> captures = mine.get_captures(one_case);
    u32 at = (u32) captures.size();

    for (u32 i = 0; i < captures.size(); i++) {
        if (std::string(module->get_token_value(
                ast->get_node(captures[i])->get_token())) == wanted) {
            at = i;
            break;
        }
    }

    if (at >= captures.size()) {
        return INVALID_TYPE;
    }

    // and the variant it takes apart, which is the callee of the pattern
    u32 pattern = ast->get_node(one_case)->get_children();
    u32 name = ast->get_node(pattern)->get_kind() == AST_CALL
                   ? ast->get_node(pattern)->get_children()
                   : 0;

    if (name != 0 && ast->get_node(name)->get_kind() == AST_DOT) {
        name = ast->get_node(ast->get_node(name)->get_children())->get_sibling();
    }

    if (name == 0) {
        return INVALID_TYPE;
    }

    std::string variant = std::string(
        module->get_token_value(ast->get_node(name)->get_token()));
    AstQuery there;

    there.set_module(holder);

    for (u32 member : there.get_members(
             theirs->get_candidate(entry->subject)->ast_node)) {
        if (there.get_declaration_name(member) != variant) {
            continue;
        }

        u32 named = theirs->candidate_of(member);
        u32 signature = named == 0 ? INVALID_TYPE
                                   : theirs->get_candidate(named)->type;

        if (signature == INVALID_TYPE
            || holder->get_types()->get_type(signature)->kind
                   != TYPE_FUNCTION) {
            return INVALID_TYPE;
        }

        std::vector<u32> carried =
            holder->get_types()->get_arguments(signature);

        // the return type is the last one, per record 0016
        carried.pop_back();

        if (at >= carried.size()) {
            return INVALID_TYPE;
        }

        // A **reference** into the value being switched over, and not a copy
        // of what it holds. It is record 0040's loop variable again and for
        // the same two reasons: a copy of a payload that owns something is an
        // allocation per match, and a payload that cannot be copied could not
        // be taken apart at all
        return types->reference(
            builder.translate(index, entry->module, carried[at]));
    }

    return INVALID_TYPE;
}

// the name a candidate was declared under, found in the scope it lives in.
// A candidate knows what it is and where its declaration is, and not what it
// is called -- the name is the symbol's, one level up
std::string TypeCollector::name_of_candidate(u32 scope, u32 candidate) {
    SymbolTable* table = module->get_symbols();

    for (u32 symbol = table->get_scope(scope)->symbols; symbol != 0;
         symbol = table->get_symbol(symbol)->sibling_or_next) {
        for (u32 one = table->get_symbol(symbol)->candidates; one != 0;
             one = table->get_candidate(one)->next_candidate) {
            if (one == candidate) {
                return std::string(module->get_strings()->get_text(
                    table->get_symbol(symbol)->name));
            }
        }
    }

    return "";
}

// whether this type is a class that owns something -- one that declares
// 'destroy' or says how it is copied. Such a thing in a union needs the
// union's lifetime managed, which record 0026's machinery does not do yet
bool TypeCollector::owns_something(u32 type) {
    TypeTable* types = module->get_types();
    Type* entry = types->get_type(types->value_of(type));

    if (entry->kind != TYPE_NAMED) {
        return false;
    }

    Module* holder = compilation->get_module(entry->module);
    u32 candidate = entry->subject;
    AstQuery theirs;

    theirs.set_module(holder);

    u32 declaration = holder->get_symbols()->get_candidate(candidate)->ast_node;
    std::string own = theirs.get_declaration_name(declaration);

    for (u32 member : theirs.get_members(declaration)) {
        std::string named = theirs.get_declaration_name(member);

        if (named == "destroy") {
            return true;
        }

        if (named != "init") {
            continue;
        }

        // a copy 'init' is the one taking this very class (record 0038), and
        // it makes the copy non-trivial whether or not there is a 'destroy'
        for (u32 parameter : theirs.get_params(member)) {
            u32 wrote = theirs.get_written_type(parameter);

            if (wrote != 0
                && holder->get_ast()->get_node(wrote)->get_kind()
                       == AST_REFERENCE_TYPE) {
                wrote = holder->get_ast()->get_node(wrote)->get_children();
            }

            if (wrote != 0
                && holder->get_ast()->get_node(wrote)->get_kind()
                       == AST_NAMED_TYPE) {
                u32 name = holder->get_ast()->get_node(wrote)->get_children();

                if (name != 0
                    && std::string(holder->get_token_value(
                           holder->get_ast()->get_node(name)->get_token()))
                           == own) {
                    return true;
                }
            }
        }
    }

    return false;
}

std::vector<u32> TypeCollector::payload_of(u32 written, u32 scope) {
    std::vector<u32> carried;

    if (module->get_ast()->get_node(written)->get_kind() == AST_TUPLE_TYPE) {
        for (u32 child = module->get_ast()->get_node(written)->get_children();
             child != 0;
             child = module->get_ast()->get_node(child)->get_sibling()) {
            carried.push_back(builder.build(index, scope, child));
        }

        return carried;
    }

    carried.push_back(builder.build(index, scope, written));

    return carried;
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

    // Record 0031: what a binding was given is **copied** into it, so a class
    // that owns something and has not said how to be copied cannot be what it
    // was given. Asked before the list below, because a type that fits
    // perfectly is exactly the one this is about
    if (!coercion.may_be_copied(index, written == INVALID_TYPE ? given
                                                              : written)) {
        report(name_node_of(node),
               typer.name_of(written == INVALID_TYPE ? given : written)
                   + " cannot be copied, and this is given one");

        return written == INVALID_TYPE ? given : written;
    }

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
