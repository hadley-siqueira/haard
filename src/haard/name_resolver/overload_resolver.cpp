#include <haard/name_resolver/overload_resolver.h>

using namespace haard;

static bool is_integer(BuiltinType which) {
    return which <= BUILTIN_I64;
}

static bool is_float(BuiltinType which) {
    return which == BUILTIN_F32 || which == BUILTIN_F64;
}

OverloadResolver::OverloadResolver() {
    compilation = nullptr;
}

void OverloadResolver::set_compilation(Compilation* compilation) {
    this->compilation = compilation;

    builder.set_compilation(compilation);
}

Overload OverloadResolver::choose(u32 caller,
                                  const std::vector<Candidacy>& candidates,
                                  const std::vector<Argument>& arguments) {
    Overload best;
    int lowest = -1;
    u32 ties = 0;

    best.status = OVERLOAD_NONE;
    best.module = 0;
    best.candidate = 0;
    best.result = INVALID_TYPE;

    for (const Candidacy& candidacy : candidates) {
        int points = score(caller, candidacy, arguments);

        if (points < 0) {
            continue;
        }

        if (lowest < 0 || points < lowest) {
            lowest = points;
            ties = 1;
            best.status = OVERLOAD_FOUND;
            best.module = candidacy.module;
            best.candidate = candidacy.candidate;
            continue;
        }

        if (points == lowest) {
            ties++;
        }
    }

    // record 0012: two candidates a call finds equally good are an ambiguous
    // call, and it is reported here and not where the name was gathered
    if (ties > 1) {
        best.status = OVERLOAD_AMBIGUOUS;

        return best;
    }

    if (best.status == OVERLOAD_FOUND) {
        Module* owner = compilation->get_module(best.module);
        u32 signature = owner->get_symbols()->get_candidate(best.candidate)->type;

        best.result = builder.translate(
            caller, best.module,
            owner->get_types()->get_arguments(signature).back());
    }

    return best;
}

int OverloadResolver::score(u32 caller, const Candidacy& candidacy,
                            const std::vector<Argument>& arguments) {
    Module* owner = compilation->get_module(candidacy.module);
    Candidate* candidate =
        owner->get_symbols()->get_candidate(candidacy.candidate);

    if (candidate->kind != SYMBOL_FUNCTION
        || candidate->type == INVALID_TYPE) {
        return -1;
    }

    std::vector<u32> signature =
        owner->get_types()->get_arguments(candidate->type);

    // the last one is the return type, which record 0012 keeps out of what
    // distinguishes two overloads
    signature.pop_back();

    u32 required = required_of(candidacy.module, candidacy.candidate);

    if (arguments.size() < required || arguments.size() > signature.size()) {
        return -1;
    }

    int total = 0;

    for (u32 i = 0; i < arguments.size(); i++) {
        int step = match(caller,
                         arguments[i],
                         builder.translate(caller, candidacy.module,
                                           signature[i]));

        if (step < 0) {
            return -1;
        }

        total += step;
    }

    return total;
}

int OverloadResolver::match(u32 caller, const Argument& argument,
                            u32 parameter) {
    if (parameter == INVALID_TYPE) {
        return -1;
    }

    TypeTable* types = compilation->get_module(caller)->get_types();
    Type* wanted = types->get_type(parameter);

    // record 0018: a literal has no type until its context gives it one, and
    // the context here is this parameter. So it is asked to be it, and the
    // question is about the value
    if (argument.literal) {
        return wanted->kind == TYPE_BUILTIN
                       && fits(caller, argument, wanted->subject)
                   ? 0
                   : -1;
    }

    if (argument.type == INVALID_TYPE) {
        return -1;
    }

    if (argument.type == parameter) {
        return 0;
    }

    // the only other thing that matches is a reference or a pointer to
    // something derived. Record 0018 keeps a plain value off this list, since
    // an upcast by value is C++'s slicing and an error here
    Type* given = types->get_type(argument.type);

    if (given->kind != wanted->kind
        || (given->kind != TYPE_POINTER && given->kind != TYPE_REFERENCE)) {
        return -1;
    }

    return distance(caller, types->get_argument(given->first_argument),
                    types->get_argument(wanted->first_argument));
}

int OverloadResolver::distance(u32 caller, u32 from, u32 to) {
    TypeTable* types = compilation->get_module(caller)->get_types();
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

        from = builder.translate(caller, entry->module, base);
        steps++;
    }

    return -1;
}

u32 OverloadResolver::required_of(u32 module, u32 candidate) {
    Module* owner = compilation->get_module(module);
    AstQuery query;
    u32 node = owner->get_symbols()->get_candidate(candidate)->ast_node;
    u32 required = 0;

    query.set_module(owner);

    // a parameter with a default may be left out, which is what makes arity a
    // range and lets two candidates both answer to one count
    for (u32 param : query.get_params(node)) {
        if (query.get_binding_expression(param) == 0) {
            required++;
        }
    }

    return required;
}

bool OverloadResolver::fits(u32 caller, const Argument& argument,
                            u32 builtin) {
    Module* module = compilation->get_module(caller);
    AstNodeKind kind =
        (AstNodeKind) module->get_ast()->get_node(argument.node)->get_kind();
    bool integer = kind == AST_INTEGER_LITERAL;

    if (integer != is_integer((BuiltinType) builtin)) {
        return integer ? false : is_float((BuiltinType) builtin);
    }

    if (!integer) {
        return is_float((BuiltinType) builtin);
    }

    // the digits against the width, which is the check about the value that
    // record 0018 asked for
    static const u64 limits[] = {
        0xff, 0xffff, 0xffffffff, 0xffffffffffffffffULL,
        0x7f, 0x7fff, 0x7fffffff, 0x7fffffffffffffffULL
    };
    std::string digits = std::string(module->get_token_value(
        module->get_ast()->get_node(argument.node)->get_token()));
    u64 value = 0;

    for (char digit : digits) {
        if (digit < '0' || digit > '9') {
            return true;
        }

        value = value * 10 + (u64) (digit - '0');

        if (value > limits[builtin]) {
            return false;
        }
    }

    return true;
}
