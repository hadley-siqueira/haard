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
        return climb(module, given, types->get_argument(to->first_argument));
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
    // destructor. Saying how to be copied is what 'copy' says
    if (!declares(entry->module, entry->subject, "destroy")) {
        return true;
    }

    return declares(entry->module, entry->subject, "copy");
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
