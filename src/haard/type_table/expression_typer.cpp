#include <haard/string_table/string_table.h>
#include <haard/type_table/expression_typer.h>

using namespace haard;

static const char* BUILTIN_NAMES[] = {
    "u8", "u16", "u32", "u64", "i8", "i16", "i32", "i64",
    "f32", "f64", "bool", "void", "char"
};

// the widest value each integer builtin holds, and 0 for the ones a literal
// cannot be asked to be
static u64 limit_of(BuiltinType which) {
    switch (which) {
    case BUILTIN_U8: return 0xff;
    case BUILTIN_U16: return 0xffff;
    case BUILTIN_U32: return 0xffffffff;
    case BUILTIN_U64: return 0xffffffffffffffffULL;
    case BUILTIN_I8: return 0x7f;
    case BUILTIN_I16: return 0x7fff;
    case BUILTIN_I32: return 0x7fffffff;
    case BUILTIN_I64: return 0x7fffffffffffffffULL;
    default: break;
    }

    return 0;
}

ExpressionTyper::ExpressionTyper() {
    compilation = nullptr;
    module = nullptr;
    index = 0;
}

void ExpressionTyper::set_compilation(Compilation* compilation) {
    this->compilation = compilation;

    resolver.set_compilation(compilation);
    overloads.set_compilation(compilation);
}

u32 ExpressionTyper::type_of(u32 index, u32 scope, u32 node, u32 expected) {
    if (node == 0) {
        return INVALID_TYPE;
    }

    this->index = index;
    module = compilation->get_module(index);

    switch (kind_of(node)) {
    case AST_INTEGER_LITERAL:
        return literal(node, expected, BUILTIN_I32);

    case AST_FLOAT_LITERAL:
        return literal(node, expected, BUILTIN_F64);

    case AST_CHAR_LITERAL:
        return literal(node, expected, BUILTIN_CHAR);

    case AST_TRUE:
    case AST_FALSE:
        return module->get_types()->builtin(BUILTIN_BOOL);

    case AST_IDENTIFIER:
        return identifier(scope, node);

    case AST_PARENTHESIS:
        return type_of(index, scope, first_child(node), expected);

    case AST_PLUS:
    case AST_MINUS:
    case AST_TIMES:
    case AST_DIVISION:
    case AST_INTEGER_DIVISION:
    case AST_MODULO:
        return binary(scope, node, expected, false);

    case AST_EQUAL:
    case AST_NOT_EQUAL:
    case AST_LESS_THAN:
    case AST_GREATER_THAN:
    case AST_LESS_THAN_OR_EQUAL:
    case AST_GREATER_THAN_OR_EQUAL:
        return binary(scope, node, INVALID_TYPE, true);

    case AST_DOT:
        return member(scope, node, false);

    case AST_ARROW:
        return member(scope, node, true);

    case AST_CALL:
        return call(scope, node);

    case AST_THIS:
        return this_type(scope);

    // a string has no type until the prelude declares one, and a method call
    // needs the member lookup to give back candidates instead of a type.
    // Neither is an error here: this phase says nothing rather than guessing,
    // and the golden shows the nothing
    default:
        break;
    }

    return INVALID_TYPE;
}

u32 ExpressionTyper::literal(u32 node, u32 expected, BuiltinType fallback) {
    TypeTable* types = module->get_types();

    // with nothing expected the literal takes its default. Record 0018: an
    // integer is an i32 and a float an f64 when no context asks otherwise
    if (expected == INVALID_TYPE) {
        return types->builtin(fallback);
    }

    Type* wanted = types->get_type(expected);

    if (wanted->kind != TYPE_BUILTIN) {
        report(node, "a literal cannot be " + name_of(expected));

        return INVALID_TYPE;
    }

    // a float literal is not an integer of any width, and an integer literal
    // becoming a float would be the conversion record 0018 does not have
    bool integer = kind_of(node) == AST_INTEGER_LITERAL;
    bool wants_integer = limit_of((BuiltinType) wanted->subject) > 0;

    if (integer != wants_integer) {
        report(node, "expected " + name_of(expected) + ", found " +
               std::string(integer ? "an integer" : "a floating point") +
               " literal");

        return INVALID_TYPE;
    }

    if (integer && !fits(node, expected)) {
        report(node, text_of(node) + " does not fit in " + name_of(expected));

        return INVALID_TYPE;
    }

    return expected;
}

u32 ExpressionTyper::identifier(u32 scope, u32 node) {
    std::vector<Candidacy> found =
        resolver.resolve(index, scope, text_of(node));

    // an unknown name is the UseResolver's diagnostic, and a name with several
    // candidates is a call to resolve, not a type to read
    if (found.size() != 1) {
        return INVALID_TYPE;
    }

    return compilation->get_module(found[0].module)
        ->get_symbols()
        ->get_candidate(found[0].candidate)
        ->type;
}

u32 ExpressionTyper::binary(u32 scope, u32 node, u32 expected,
                            bool comparison) {
    u32 left = type_of(index, scope, first_child(node), expected);
    u32 right = type_of(index, scope, second_child(node), left);

    if (left == INVALID_TYPE || right == INVALID_TYPE) {
        return INVALID_TYPE;
    }

    // record 0018 has nothing that would make two different types one, so the
    // operator is where that is said
    if (left != right) {
        report(node, "cannot apply this to " + name_of(left) + " and " +
               name_of(right));

        return INVALID_TYPE;
    }

    return comparison ? module->get_types()->builtin(BUILTIN_BOOL) : left;
}

std::vector<Candidacy> ExpressionTyper::callee_of(u32 scope, u32 node) {
    // a call written with explicit generic arguments has an AST_GENERIC_NAME
    // for a callee, and the name is its first child
    if (kind_of(node) == AST_GENERIC_NAME) {
        node = first_child(node);
    }

    if (kind_of(node) == AST_IDENTIFIER) {
        return resolver.resolve(index, scope, text_of(node));
    }

    if (kind_of(node) == AST_SCOPE) {
        u32 first = first_child(node);
        u32 second = module->get_ast()->get_node(first)->get_sibling();

        if (second == 0) {
            return resolver.resolve_at_module(index, text_of(first));
        }

        return resolver.resolve_qualified(index, text_of(first),
                                          text_of(second));
    }

    // a method call: the same walk a field access does, giving back the set
    // instead of the first answer's type
    if (kind_of(node) == AST_DOT || kind_of(node) == AST_ARROW) {
        u32 left = type_of(index, scope, first_child(node), INVALID_TYPE);
        u32 name = second_child(node);

        if (left == INVALID_TYPE || name == 0) {
            return std::vector<Candidacy>();
        }

        TypeTable* types = module->get_types();
        Type* entry = types->get_type(left);
        bool pointer = entry->kind == TYPE_POINTER;

        if (kind_of(node) == AST_ARROW && !pointer) {
            report(name, "'->' needs a pointer, and this is " + name_of(left));

            return std::vector<Candidacy>();
        }

        if (pointer) {
            left = types->get_argument(entry->first_argument);
        }

        u32 owner = index;
        std::vector<Candidacy> found = members_of(left, name, owner);

        // nobody else can say this. The UseResolver skips the right side of a
        // dot on purpose, so an unknown method is only ever reported here
        if (found.size() == 0) {
            report(name, name_of(left) + " has no member named '" +
                   text_of(name) + "'");
        }

        return found;
    }

    return std::vector<Candidacy>();
}

u32 ExpressionTyper::call(u32 scope, u32 node) {
    u32 callee = first_child(node);
    u32 list = second_child(node);
    std::vector<Candidacy> candidates = callee_of(scope, callee);
    std::vector<Argument> arguments;

    if (candidates.size() == 0) {
        return INVALID_TYPE;
    }

    for (u32 child = list == 0 ? 0 : first_child(list); child != 0;
         child = module->get_ast()->get_node(child)->get_sibling()) {
        Argument argument;
        AstNodeKind kind = kind_of(child);

        // record 0018: a literal has no type of its own, so it is carried
        // untyped and each candidate asks it to be its own parameter. Typing
        // it here would make 'f(3)' pick i32 and then fail against 'f(u8)'
        argument.literal = kind == AST_INTEGER_LITERAL
                        || kind == AST_FLOAT_LITERAL;
        argument.node = child;
        argument.type = argument.literal
                            ? INVALID_TYPE
                            : type_of(index, scope, child, INVALID_TYPE);

        arguments.push_back(argument);
    }

    Overload chosen = overloads.choose(index, candidates, arguments);

    // a dot carries no text of its own, so the name being called is its right
    // side -- which is also the span a reader wants underlined
    u32 at = kind_of(callee) == AST_DOT || kind_of(callee) == AST_ARROW
                 ? second_child(callee)
                 : callee;

    if (chosen.status == OVERLOAD_AMBIGUOUS) {
        report(at, "this call matches more than one '" + text_of(at) +
               "' equally well");

        return INVALID_TYPE;
    }

    if (chosen.status == OVERLOAD_NONE) {
        report(at, "no '" + text_of(at) + "' takes these arguments");

        return INVALID_TYPE;
    }

    return chosen.result;
}

std::vector<Candidacy> ExpressionTyper::members_of(u32 left, u32 name,
                                                  u32& owner) {
    std::vector<Candidacy> found;
    u32 declaration = class_of(left, owner);

    if (declaration == 0) {
        return found;
    }

    Module* holder = compilation->get_module(owner);
    std::string wanted = text_of(name);
    u32 interned = holder->get_strings()->find(hash_name(wanted), wanted);

    // up the chain of bases, one step at a time. Single inheritance (Hadley,
    // 2026-09-02, and no interfaces) makes this a walk and never a search
    while (declaration != 0) {
        SymbolTable* table = holder->get_symbols();
        Candidate* holder_candidate = table->get_candidate(declaration);
        u32 body = table->scope_owned_by(holder_candidate->ast_node);
        u32 symbol = interned == INVALID_STRING || body == 0
                         ? 0
                         : table->find(body, interned);

        // Record 0012's scope steps contribute to one candidate set, and a
        // base is one of those steps: a method a base declares is an overload
        // of one the derived class declares, not something it hides
        for (u32 candidate = symbol == 0
                                 ? 0
                                 : table->get_symbol(symbol)->candidates;
             candidate != 0;
             candidate = table->get_candidate(candidate)->next_candidate) {
            found.push_back(Candidacy{owner, candidate});
        }

        u32 base = holder_candidate->super;

        if (base == INVALID_TYPE) {
            break;
        }

        // the base may live in another module, and then the name has to be
        // interned there before it means anything -- record 0013's rule for a
        // lookup that crosses an import
        u32 next = owner;

        declaration = class_of(base, next);

        if (next != owner) {
            owner = next;
            holder = compilation->get_module(owner);
            interned = holder->get_strings()->find(hash_name(wanted), wanted);
        }
    }

    return found;
}

u32 ExpressionTyper::this_type(u32 scope) {
    SymbolTable* table = module->get_symbols();

    // outward until a scope a type declaration opened. A method's own scope is
    // inside its class's, so this is the class the method was written in
    for (u32 current = scope; current != 0;
         current = table->get_scope(current)->parent) {
        u32 owner = table->get_scope(current)->owner;

        if (owner == 0) {
            continue;
        }

        switch (kind_of(owner)) {
        case AST_CLASS:
        case AST_STRUCT:
        case AST_UNION:
        case AST_ENUM:
            return module->get_types()->pointer(
                table->get_candidate(table->candidate_of(owner))->type);

        default:
            break;
        }
    }

    return INVALID_TYPE;
}

u32 ExpressionTyper::member(u32 scope, u32 node, bool through_pointer) {
    u32 left = type_of(index, scope, first_child(node), INVALID_TYPE);
    u32 name = second_child(node);

    if (left == INVALID_TYPE || name == 0) {
        return INVALID_TYPE;
    }

    TypeTable* types = module->get_types();
    Type* entry = types->get_type(left);
    bool pointer = entry->kind == TYPE_POINTER;

    // Record 0018's third coercion, and it is not C++'s split. '.' reads a
    // member of a T and of a T* alike, and on a pointer it means what '->'
    // means. '->' is the explicit form and only a pointer may be written with
    // it. Both look through exactly one level, so a T** has members under
    // neither
    if (through_pointer && !pointer) {
        report(name, "'->' needs a pointer, and this is " + name_of(left));

        return INVALID_TYPE;
    }

    if (pointer) {
        left = types->get_argument(entry->first_argument);
    }

    u32 owner = index;
    std::vector<Candidacy> found = members_of(left, name, owner);

    if (found.size() == 0) {
        report(name, name_of(left) + " has no member named '" +
               text_of(name) + "'");

        return INVALID_TYPE;
    }

    return compilation->get_module(found[0].module)
        ->get_symbols()
        ->get_candidate(found[0].candidate)
        ->type;
}

u32 ExpressionTyper::class_of(u32 type, u32& owner) {
    TypeTable* types = compilation->get_module(owner)->get_types();
    Type* entry = types->get_type(type);

    // a reference is the thing it refers to, for this question
    if (entry->kind == TYPE_REFERENCE) {
        entry = types->get_type(types->get_argument(entry->first_argument));
    }

    if (entry->kind != TYPE_NAMED) {
        return 0;
    }

    owner = entry->module;

    return entry->subject;
}

bool ExpressionTyper::fits(u32 node, u32 type) {
    TypeTable* types = module->get_types();
    u64 limit = limit_of((BuiltinType) types->get_type(type)->subject);
    u64 value = 0;
    std::string digits = text_of(node);

    for (char digit : digits) {
        if (digit < '0' || digit > '9') {
            // a literal written in another base, or with '_' separators, is
            // not read here yet. Saying nothing beats saying something wrong
            return true;
        }

        value = value * 10 + (u64) (digit - '0');

        if (value > limit) {
            return false;
        }
    }

    return true;
}

void ExpressionTyper::report(u32 node, const std::string& message) {
    Token& token = module->get_tokens()->get_token(
        module->get_ast()->get_node(node)->get_token());

    module->get_logger()->error(token.get_offset(), token.get_length(),
                                message);
}

std::string ExpressionTyper::text_of(u32 node) {
    return std::string(module->get_token_value(
        module->get_ast()->get_node(node)->get_token()));
}

AstNodeKind ExpressionTyper::kind_of(u32 node) {
    return (AstNodeKind) module->get_ast()->get_node(node)->get_kind();
}

u32 ExpressionTyper::first_child(u32 node) {
    return module->get_ast()->get_node(node)->get_children();
}

u32 ExpressionTyper::second_child(u32 node) {
    u32 first = first_child(node);

    return first == 0 ? 0 : module->get_ast()->get_node(first)->get_sibling();
}

std::string ExpressionTyper::name_of(u32 type) {
    Type* entry = module->get_types()->get_type(type);
    AstQuery query;

    if (entry->kind == TYPE_BUILTIN && entry->subject < BUILTIN_COUNT) {
        return BUILTIN_NAMES[entry->subject];
    }

    if (entry->kind == TYPE_POINTER) {
        return name_of(module->get_types()->get_argument(
                   entry->first_argument)) + "*";
    }

    // a named type says the name it was declared with, which is the only
    // spelling a reader of the diagnostic would recognise
    if (entry->kind == TYPE_NAMED) {
        Module* owner = compilation->get_module(entry->module);

        query.set_module(owner);

        return query.get_declaration_name(
            owner->get_symbols()->get_candidate(entry->subject)->ast_node);
    }

    return "type " + std::to_string(type);
}
