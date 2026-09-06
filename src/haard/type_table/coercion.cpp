#include <haard/ast_query/ast_query.h>
#include <haard/type_table/coercion.h>

using namespace haard;

Coercion::Coercion() {
    compilation = nullptr;
}

void Coercion::set_compilation(Compilation* compilation) {
    this->compilation = compilation;

    builder.set_compilation(compilation);
}

bool Coercion::fits(u32 module, u32 given, u32 wanted) {
    return steps(module, given, wanted) >= 0;
}

int Coercion::steps(u32 module, u32 given, u32 wanted) {
    if (given == INVALID_TYPE || wanted == INVALID_TYPE) {
        return -1;
    }

    // record 0016 writes every type once, so the same type is the same index
    // and the commonest question in the language is a comparison of two u32s
    if (given == wanted) {
        return 0;
    }

    TypeTable* types = compilation->get_module(module)->get_types();
    Type* from = types->get_type(given);
    Type* to = types->get_type(wanted);

    // record 0018: a reference or a pointer to something derived. A plain
    // value is deliberately not here -- an upcast by value copies the base
    // part and discards the rest, which is C++'s slicing and an error
    if (from->kind == to->kind
        && (from->kind == TYPE_POINTER || from->kind == TYPE_REFERENCE)) {
        return climb(module, types->get_argument(from->first_argument),
                     types->get_argument(to->first_argument));
    }

    // A **value where a reference to it was asked for**, which is not a
    // conversion at all: the parameter *is* the value, and nothing happens at
    // run time. Record 0018's list did not have it and record 0031 is what
    // made that show: 0031 sends anyone holding something that owns memory
    // towards a '&' parameter to avoid the copy, and a '&' parameter could
    // not be passed a value, so there was nowhere to go.
    //
    // A derived value binds to a base's reference for the same reason the
    // list already allows it between two references -- nothing is sliced,
    // because nothing is copied.
    //
    if (to->kind == TYPE_REFERENCE && from->kind != TYPE_REFERENCE) {
        int climbed =
            climb(module, given, types->get_argument(to->first_argument));

        // and when it is not a class above this one, the entries below still
        // get their turn -- 'char*' where a 'String&' was asked for is one of
        // them, and returning here made it unreachable
        if (climbed >= 0) {
            return climbed;
        }
    }

    // And the other direction, added 2026-09-06 with Hadley's 'implemente T&
    // com semântica parecida com C++'. **A reference IS the thing it names**,
    // so reading one is reading what it names, and without this a 'T&' could
    // not be given back from a function usefully: 'total + xs.at(i)' was
    // *cannot apply this to i32 and i32&*, and so were a binding, a call, a
    // comparison and an assignment. Six places, and 'at' returning a
    // reference is what makes 'a[i] = x' possible at all.
    //
    // Unlike the direction above, this one is **not free**: it is a copy. So
    // record 0031's question is asked, and a class that owns something and
    // says nothing about being copied cannot be read out of a reference any
    // more than it can be passed by value.
    //
    // It costs a step, so an overload taking the reference still wins over
    // one taking the value when a reference is what was passed
    if (from->kind == TYPE_REFERENCE && to->kind != TYPE_REFERENCE) {
        u32 named = types->get_argument(from->first_argument);

        if (!may_be_copied(module, named)) {
            return -1;
        }

        int rest = steps(module, named, wanted);

        return rest < 0 ? -1 : rest + 1;
    }

    // A fixed array where a pointer to its element was asked for, which is
    // C's decay and C++'s. Record 0036's successor needs it: a '{1, 2, 3}' is
    // an 'i32[3]' and the constructor that takes one is 'init(T*, i32)', so
    // without this the literal cannot reach the class it is written for.
    //
    // Free, and only to the element's own type -- a decay is an address and
    // not a conversion, so nothing is climbed and nothing is copied
    if (from->kind == TYPE_ARRAY && to->kind == TYPE_POINTER) {
        return types->get_argument(from->first_argument)
                       == types->get_argument(to->first_argument)
                   ? 1
                   : -1;
    }

    // Agenda 1.21, Hadley 2026-09-03: a 'char*' where a 'String' was asked
    // for. It is the first entry on record 0018's list that is a **library**
    // relation and not a language one, and the only one that is not free --
    // it names a class the compiler otherwise knows nothing about, and it runs
    // String's constructor, which allocates and copies.
    //
    // Costing one, and that is the whole of the ranking agenda 1.21 asked for:
    // record 0022 makes a string literal a 'char*' when nothing asks, so with
    // both 'write(char*)' and 'write(String)' in view the first is exact and
    // the second is a step away. 'char* first' falls out of the list instead
    // of being written into the ranking as an exception
    if (is_char_pointer(module, given) && is_string(module, wanted)) {
        return 1;
    }

    // And the same thing where a **reference** to a String was asked for.
    // Hadley, 2026-09-06: 'foo("abc")' against 'foo(@s : String&)' builds a
    // String and passes it, which is one construction and not two steps -- so
    // this is one entry of its own and NOT record 0018's list learning to
    // compose. Whether it should compose is still open (record 0035).
    //
    // Costing two, so that a 'foo(String&)' next to a 'foo(char*)' still gives
    // the literal to the second: what took a step outranks what took none, and
    // this took one more than the entry above
    if (is_char_pointer(module, given) && to->kind == TYPE_REFERENCE
        && is_string(module, types->get_argument(to->first_argument))) {
        return 2;
    }

    return -1;
}

int Coercion::climb(u32 module, u32 from, u32 to) {
    TypeTable* types = compilation->get_module(module)->get_types();
    int steps = 0;

    // single inheritance (Hadley, 2026-09-02, and no interfaces) is what makes
    // this a walk up a chain instead of a search through a graph
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

        from = builder.translate(module, entry->module, base);
        steps++;
    }

    return -1;
}

bool Coercion::is_char_pointer(u32 module, u32 type) {
    TypeTable* types = compilation->get_module(module)->get_types();
    Type* entry = types->get_type(type);

    if (entry->kind != TYPE_POINTER) {
        return false;
    }

    Type* pointed =
        types->get_type(types->get_argument(entry->first_argument));

    return pointed->kind == TYPE_BUILTIN && pointed->subject == BUILTIN_CHAR;
}

bool Coercion::is_string(u32 module, u32 type) {
    TypeTable* types = compilation->get_module(module)->get_types();
    Type* entry = types->get_type(type);

    // a generic instantiation is not it: record 0022 says String is the one
    // class of the standard library that is not generic
    if (entry->kind != TYPE_NAMED || entry->argument_count > 0) {
        return false;
    }

    Module* holder = compilation->get_module(entry->module);
    Candidate* found = holder->get_symbols()->get_candidate(entry->subject);
    AstQuery query;

    query.set_module(holder);

    return query.get_declaration_name(found->ast_node) == "String";
}

bool Coercion::may_be_copied(u32 module, u32 type) {
    if (type == INVALID_TYPE) {
        return true;
    }

    Type* entry = compilation->get_module(module)->get_types()->get_type(type);

    // a pointer and a reference name a thing rather than holding one, and a
    // builtin owns nothing. Only a class held by value is copied
    if (entry->kind != TYPE_NAMED) {
        return true;
    }

    // owning is what 'destroy' says, since record 0026 makes it the
    // destructor. Saying how to be copied is an 'init' taking one of these
    if (!declares(entry->module, entry->subject, "destroy")) {
        return true;
    }

    return declares_a_copy(entry->module, entry->subject);
}

bool Coercion::declares_a_copy(u32 module, u32 candidate) {
    Module* holder = compilation->get_module(module);
    SymbolTable* table = holder->get_symbols();
    Ast* ast = holder->get_ast();
    AstQuery query;

    query.set_module(holder);

    u32 declaration = table->get_candidate(candidate)->ast_node;
    std::string mine = query.get_declaration_name(declaration);

    // a class's name is not where a function's is, and the query gives back
    // nothing for one -- the same fallback the emitter's 'name_of' makes
    if (mine.size() == 0) {
        mine = std::string(
            holder->get_token_value(ast->get_node(declaration)->get_token()));
    }

    for (u32 member : query.get_members(declaration)) {
        if (query.get_declaration_name(member) != "init") {
            continue;
        }

        std::vector<u32> params = query.get_params(member);

        if (params.size() != 1) {
            continue;
        }

        // The type as it was **written**, and not as it was built. This is
        // asked while a binding is being inferred, and a generic's clone is
        // made in that same pass -- its 'init' gets a signature from a later
        // sweep, so a type-based test answers 'no copy' for every
        // 'let xs = [1, 2, 3]'. The name is there from the parser.
        //
        // A parameter's children are a binding NAME and a binding TYPE, so
        // the type sits one level further down than a reader expects
        u32 written = 0;

        for (u32 child = ast->get_node(params[0])->get_children(); child != 0;
             child = ast->get_node(child)->get_sibling()) {
            if (ast->get_node(child)->get_kind() == AST_BINDING_TYPE) {
                written = ast->get_node(child)->get_children();
                break;
            }
        }

        // by value or by reference is the whole of the shape, so one
        // reference is peeled and nothing else is
        if (written != 0
            && ast->get_node(written)->get_kind() == AST_REFERENCE_TYPE) {
            written = ast->get_node(written)->get_children();
        }

        if (written == 0
            || ast->get_node(written)->get_kind() != AST_NAMED_TYPE) {
            continue;
        }

        u32 name = ast->get_node(written)->get_children();

        if (name != 0
            && std::string(holder->get_token_value(
                   ast->get_node(name)->get_token())) == mine) {
            return true;
        }
    }

    return false;
}

bool Coercion::declares(u32 module, u32 candidate, const std::string& name) {
    std::vector<u32> seen;

    while (candidate != 0) {
        Module* holder = compilation->get_module(module);
        SymbolTable* table = holder->get_symbols();
        u32 declaration = table->get_candidate(candidate)->ast_node;
        AstQuery query;

        query.set_module(holder);

        for (u32 member : query.get_members(declaration)) {
            if (query.get_declaration_name(member) == name) {
                return true;
            }
        }

        u32 above = table->get_candidate(candidate)->super;

        if (above == INVALID_TYPE) {
            return false;
        }

        // the base's own module and its own table, which is where its type
        // index means anything -- record 0016
        Type* entry = holder->get_types()->get_type(above);

        if (entry->kind != TYPE_NAMED) {
            return false;
        }

        u32 key = entry->module * 1000003 + entry->subject;

        // a cycle in the bases, which nothing rejects yet. Stopping keeps the
        // walk finite and is not a diagnostic
        for (u32 already : seen) {
            if (already == key) {
                return false;
            }
        }

        seen.push_back(key);

        module = entry->module;
        candidate = entry->subject;
    }

    return false;
}
