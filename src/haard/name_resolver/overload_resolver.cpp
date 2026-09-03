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
    coercion.set_compilation(compilation);
}

Overload OverloadResolver::choose(u32 caller,
                                  const std::vector<Candidacy>& candidates,
                                  const std::vector<Argument>& arguments) {
    Overload best;
    int lowest = -1;

    // those tied at the lowest score, and not merely how many. An override
    // has to be told from the method it overrides, and that needs the
    // candidates themselves
    std::vector<Candidacy> tied;

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
            tied.clear();
        }

        if (points == lowest) {
            tied.push_back(candidacy);
        }
    }

    // a method the derived class wrote over one of its bases is not a second
    // candidate: it is the same method written again, and the derived one is
    // what a call means. Without this the most ordinary class in the language
    // -- one that reimplements something -- cannot be called at all
    bool again = true;

    while (again) {
        again = false;

        for (u32 i = 0; i < tied.size() && !again; i++) {
            for (u32 j = 0; j < tied.size(); j++) {
                if (i != j && overrides(caller, tied[i], tied[j])) {
                    tied.erase(tied.begin() + j);
                    again = true;
                    break;
                }
            }
        }
    }

    if (tied.size() > 0) {
        best.status = OVERLOAD_FOUND;
        best.module = tied[0].module;
        best.candidate = tied[0].candidate;
    }

    // record 0012: two candidates a call finds equally good are an ambiguous
    // call, and it is reported here and not where the name was gathered
    if (tied.size() > 1) {
        best.status = OVERLOAD_AMBIGUOUS;

        return best;
    }

    if (best.status == OVERLOAD_FOUND) {
        Module* owner = compilation->get_module(best.module);
        u32 signature = owner->get_symbols()->get_candidate(best.candidate)->type;
        std::vector<u32> written = owner->get_types()->get_arguments(signature);

        best.result = builder.translate(caller, best.module, written.back());

        // the return is the last one, per record 0016, and what is left is
        // the parameters the caller's side has to be able to name
        written.pop_back();

        for (u32 parameter : written) {
            best.parameters.push_back(
                builder.translate(caller, best.module, parameter));
        }
    }

    return best;
}

bool OverloadResolver::overrides(u32 caller, const Candidacy& derived,
                                 const Candidacy& base) {
    u32 below = holder_of(derived);
    u32 above = holder_of(base);

    // both have to be methods, and of two different classes
    if (below == INVALID_TYPE || above == INVALID_TYPE) {
        return false;
    }

    // strictly below: the same class twice is two real overloads that happen
    // to tie, which is record 0012's ambiguous call and not an override
    if (coercion.climb(caller, below, above) <= 0) {
        return false;
    }

    return parameters_of(caller, derived) == parameters_of(caller, base);
}

u32 OverloadResolver::holder_of(const Candidacy& who) {
    Module* owner = compilation->get_module(who.module);
    SymbolTable* table = owner->get_symbols();
    Candidate* candidate = table->get_candidate(who.candidate);

    if (candidate->kind != SYMBOL_FUNCTION) {
        return INVALID_TYPE;
    }

    // the scope the function opened, then out one step to whatever holds it.
    // A free function's parent is the module scope, which owns nothing
    u32 inside = table->scope_owned_by(candidate->ast_node);

    if (inside == 0) {
        return INVALID_TYPE;
    }

    u32 around = table->get_scope(inside)->parent;
    u32 holder = around == 0 ? 0 : table->get_scope(around)->owner;

    if (holder == 0) {
        return INVALID_TYPE;
    }

    u32 declaration = table->candidate_of(holder);

    return declaration == 0 ? INVALID_TYPE
                            : table->get_candidate(declaration)->type;
}

std::vector<u32> OverloadResolver::parameters_of(u32 caller,
                                                 const Candidacy& who) {
    Module* owner = compilation->get_module(who.module);
    Candidate* candidate = owner->get_symbols()->get_candidate(who.candidate);
    std::vector<u32> written;

    if (candidate->type == INVALID_TYPE) {
        return written;
    }

    written = owner->get_types()->get_arguments(candidate->type);

    // the return is the last one and record 0012 keeps it out of what makes
    // two overloads different, so it is out of what makes one an override too
    written.pop_back();

    for (u32& one : written) {
        one = builder.translate(caller, who.module, one);
    }

    return written;
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

    // record 0018: a literal has no type until its context gives it one, and
    // the context here is this parameter. So it is asked to be it, and the
    // question is about the value
    if (argument.literal) {
        Type* wanted =
            compilation->get_module(caller)->get_types()->get_type(parameter);

        if (wanted->kind != TYPE_BUILTIN
            || !fits(caller, argument, wanted->subject)) {
            return -1;
        }

        // Hadley, 2026-09-03. Record 0018 gives every literal a type it has
        // when nothing asks -- an i32, an f64 -- and record 0022 gives a
        // string literal one too. Being asked to be that default costs
        // nothing and being asked to be anything else costs a step, which is
        // the ranking agenda 1.21 needed and it is rule 5's integer again
        // rather than an exception to it.
        //
        // It loosens: 'f(3)' between 'f(u8)' and 'f(i32)' was an ambiguous
        // call and now picks the i32. Loosening is the direction record 0018
        // says is safe, because every program that compiled still does
        return wanted->subject == default_of(caller, argument) ? 0 : 1;
    }

    return coercion.steps(caller, argument.type, parameter);
}

// what a literal is when no parameter asks it to be anything: record 0018 for
// the numbers. A character literal is not carried in untyped -- there is one
// builtin it could be -- so it does not reach here
u32 OverloadResolver::default_of(u32 caller, const Argument& argument) {
    Module* module = compilation->get_module(caller);
    AstNodeKind kind =
        (AstNodeKind) module->get_ast()->get_node(argument.node)->get_kind();

    return kind == AST_FLOAT_LITERAL ? BUILTIN_F64 : BUILTIN_I32;
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
