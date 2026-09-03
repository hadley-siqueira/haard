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
