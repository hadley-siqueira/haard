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
    builder.set_compilation(compilation);
}

void ExpressionTyper::set_module(u32 index) {
    this->index = index;
    module = compilation->get_module(index);
}

u32 ExpressionTyper::type_of(u32 index, u32 scope, u32 node, u32 expected) {
    if (node == 0) {
        return INVALID_TYPE;
    }

    this->index = index;
    module = compilation->get_module(index);

    u32 result = work(scope, node, expected);

    // Record 0019, and it is one line because every expression goes through
    // here. The kinds below work the answer out and hand it back; this is the
    // only place it is written down, so a kind added later is recorded for
    // free and none can be forgotten
    module->get_resolutions()->set_type(node, result);

    return result;
}

u32 ExpressionTyper::work(u32 scope, u32 node, u32 expected) {
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

    // 'and' and 'or', written as words or as symbols
    case AST_LOGICAL_AND:
    case AST_LOGICAL_OR:
        return logical(scope, node, false);

    // 'not' and '!', which the parser tells apart so the printer can write
    // back whichever was written. They ask the same question
    case AST_LOGICAL_NOT:
    case AST_LOGICAL_NOT_OPERATOR:
        return logical(scope, node, true);

    case AST_DOT:
        return member(scope, node, false);

    case AST_ARROW:
        return member(scope, node, true);

    case AST_CALL:
        return call(scope, node);

    case AST_THIS:
        return this_type(scope);

    case AST_INDEX:
        return subscript(scope, node);

    case AST_ADDRESS_OF:
        return address_of(scope, node);

    case AST_DEREFERENCE:
        return dereference(scope, node);

    // a unary operator gives back what it was applied to, and hands the
    // context down on the way in -- so the '1' of '-1' in an i64 place is an
    // i64 literal and never a converted i32
    case AST_UNARY_MINUS:
    case AST_UNARY_PLUS:
    case AST_BITWISE_NOT:
    case AST_PRE_INCREMENT:
    case AST_PRE_DECREMENT:
    case AST_POST_INCREMENT:
    case AST_POST_DECREMENT:
        return type_of(index, scope, first_child(node), expected);

    case AST_CAST:
        return cast(scope, node);

    case AST_NEW:
        return allocation(scope, node);

    // a size is a size whatever was measured, and the operand is typed for
    // the sake of what the recording keeps rather than for an answer
    case AST_SIZEOF:
        type_of(index, scope, first_child(node), INVALID_TYPE);

        return module->get_types()->builtin(BUILTIN_U64);

    // Nothing typed a 'delete' until 2026-09-03 -- 'delete 5' passed in
    // silence -- and the emitter found it by refusing to name an operand the
    // type phase had never looked at. It gives back void: it is written for
    // what it does, like a call whose answer is thrown away
    case AST_DELETE:
    case AST_DELETE_ARRAY: {
        u32 operand = type_of(index, scope, first_child(node), INVALID_TYPE);

        if (operand != INVALID_TYPE
            && module->get_types()->get_type(operand)->kind != TYPE_POINTER) {
            report(node, "only a pointer can be deleted, and this is "
                   + name_of(operand));
        }

        return module->get_types()->builtin(BUILTIN_VOID);
    }

    case AST_NULL_LITERAL:
        return null_literal(node, expected);

    // Record 0022: a string literal is a 'char*' first. 'char' is a builtin
    // and a pointer to one needs nothing from the standard library, which is
    // why this types before any of the standard library exists.
    //
    // Hadley, 2026-09-02: try char* first, and become a String when that is
    // not possible. The second half waits on a String to become -- and it is
    // record 0018's first rule again, so loosening it later is additive
    case AST_STRING_LITERAL:
        return module->get_types()->pointer(
            module->get_types()->builtin(BUILTIN_CHAR));

    case AST_LIST:
        return sequence(scope, node, expected, false);

    case AST_ARRAY:
        return sequence(scope, node, expected, true);

    case AST_TUPLE:
        return tuple(scope, node, expected);

    // A template string is a String and a symbol is nobody has said what,
    // and a range has no type at all -- nothing has decided whether it is one
    // or only a thing a 'for ... in' reads. All three say nothing rather than
    // guess, and the golden of
    // tests/type_table/cases/every_expression_kind shows the nothing so that
    // it is a line somebody can see rather than a silence
    default:
        break;
    }

    return INVALID_TYPE;
}

u32 ExpressionTyper::element_of(u32 type) {
    TypeTable* types = module->get_types();
    Type* entry = types->get_type(type);

    switch ((TypeKind) entry->kind) {
    case TYPE_ARRAY:
    case TYPE_LIST:
    case TYPE_POINTER:
        return types->get_argument(entry->first_argument);

    // a hash is read by its key and gives back its value, so the thing it
    // holds -- for the purpose of a subscript -- is the second of the two
    case TYPE_HASH:
        return types->get_argument(entry->first_argument + 1);

    default:
        break;
    }

    return INVALID_TYPE;
}

u32 ExpressionTyper::subscript(u32 scope, u32 node) {
    u32 left = type_of(index, scope, first_child(node), INVALID_TYPE);

    if (left == INVALID_TYPE) {
        return INVALID_TYPE;
    }

    TypeTable* types = module->get_types();
    Type* entry = types->get_type(left);
    u32 element = element_of(left);

    if (element == INVALID_TYPE) {
        report(node, name_of(left) + " cannot be indexed");

        return INVALID_TYPE;
    }

    // a hash is the one that says what its subscript has to be. For everything
    // else the subscript is a position, and nothing yet says it must be an
    // integer
    type_of(index, scope, second_child(node),
            entry->kind == TYPE_HASH
                ? types->get_argument(entry->first_argument)
                : INVALID_TYPE);

    return element;
}

u32 ExpressionTyper::address_of(u32 scope, u32 node) {
    u32 inner = type_of(index, scope, first_child(node), INVALID_TYPE);

    return inner == INVALID_TYPE ? INVALID_TYPE
                                 : module->get_types()->pointer(inner);
}

u32 ExpressionTyper::dereference(u32 scope, u32 node) {
    u32 inner = type_of(index, scope, first_child(node), INVALID_TYPE);

    if (inner == INVALID_TYPE) {
        return INVALID_TYPE;
    }

    TypeTable* types = module->get_types();
    Type* entry = types->get_type(inner);

    if (entry->kind != TYPE_POINTER) {
        report(node, "'*' needs a pointer, and this is " + name_of(inner));

        return INVALID_TYPE;
    }

    return types->get_argument(entry->first_argument);
}

u32 ExpressionTyper::cast(u32 scope, u32 node) {
    // typed for the recording's sake and not for the answer: what a cast is,
    // is what it was written as
    type_of(index, scope, first_child(node), INVALID_TYPE);

    return builder.build(index, scope, second_child(node));
}

u32 ExpressionTyper::allocation(u32 scope, u32 node) {
    u32 made = builder.build(index, scope, first_child(node));
    u32 list = second_child(node);

    // record 0026: 'new T(...)' runs T's 'init', so the arguments are a call
    // like any other and are checked like one. Nothing checked them at all
    // until 2026-09-03 -- 'new Counter(2.5)' against an 'init' taking an i32
    // passed in silence, and so did arguments to a class with no 'init'
    initialisation(scope, node, made, list);

    // record 0016's poison rule: a type that would not build is not a pointer
    // to nothing, it is nothing
    return made == INVALID_TYPE ? INVALID_TYPE
                                : module->get_types()->pointer(made);
}

// The 'init' of one class, and only its own: a base's runs on its own before
// this one, the way C++ and every language with a constructor chain does it,
// so the arguments written here answer to this class alone
std::vector<Candidacy> ExpressionTyper::constructors_of(u32 type, u32& owner) {
    std::vector<Candidacy> found;
    u32 declaration = class_of(type, owner);

    if (declaration == 0) {
        return found;
    }

    Module* holder = compilation->get_module(owner);
    SymbolTable* table = holder->get_symbols();
    std::string wanted = "init";
    u32 interned = holder->get_strings()->find(hash_name(wanted), wanted);
    u32 body = table->scope_owned_by(
        table->get_candidate(declaration)->ast_node);
    u32 symbol = interned == INVALID_STRING || body == 0
                     ? 0
                     : table->find(body, interned);

    for (u32 candidate = symbol == 0 ? 0
                                     : table->get_symbol(symbol)->candidates;
         candidate != 0;
         candidate = table->get_candidate(candidate)->next_candidate) {
        if (table->get_candidate(candidate)->kind == SYMBOL_FUNCTION) {
            found.push_back(Candidacy{owner, candidate});
        }
    }

    return found;
}

void ExpressionTyper::initialisation(u32 scope, u32 node, u32 made, u32 list) {
    std::vector<Argument> arguments;
    u32 owner = index;
    std::vector<Candidacy> candidates;
    u32 count = 0;

    for (u32 child = list == 0 ? 0 : first_child(list); child != 0;
         child = module->get_ast()->get_node(child)->get_sibling()) {
        Argument argument;
        AstNodeKind kind = kind_of(child);

        // carried untyped for the same reason a call's arguments are: record
        // 0018 has the literal take the parameter's type rather than its own
        argument.literal = kind == AST_INTEGER_LITERAL
                        || kind == AST_FLOAT_LITERAL;
        argument.node = child;
        argument.type = argument.literal
                            ? INVALID_TYPE
                            : type_of(index, scope, child, INVALID_TYPE);

        arguments.push_back(argument);
        count++;
    }

    if (made == INVALID_TYPE) {
        return;
    }

    candidates = constructors_of(made, owner);

    // a class that declares no 'init' is an aggregate and takes nothing.
    // Saying so is the difference between naming the mistake and letting the
    // arguments evaporate
    if (candidates.size() == 0) {
        if (count > 0) {
            report(node, name_of(made) + " declares no 'init', so it takes no "
                   "arguments here");
        }

        return;
    }

    Overload chosen = overloads.choose(index, candidates, arguments);

    if (chosen.status == OVERLOAD_AMBIGUOUS) {
        report(node, "this matches more than one 'init' of " + name_of(made)
               + " equally well");

        return;
    }

    if (chosen.status == OVERLOAD_NONE) {
        report(node, "no 'init' of " + name_of(made) + " takes these "
               "arguments");

        return;
    }

    // record 0019: which 'init' this construction meant, written on the node
    // that spelled the type. The emitter needs it, and nothing could work it
    // out again -- it was the arguments that chose
    module->get_resolutions()->set_declaration(node, chosen.module,
                                               chosen.candidate);

    for (u32 i = 0; i < arguments.size() && i < chosen.parameters.size();
         i++) {
        if (arguments[i].literal) {
            module->get_resolutions()->set_type(arguments[i].node,
                                                chosen.parameters[i]);
        }
    }
}

u32 ExpressionTyper::null_literal(u32 node, u32 expected) {
    if (expected != INVALID_TYPE
        && module->get_types()->get_type(expected)->kind == TYPE_POINTER) {
        return expected;
    }

    report(node, expected == INVALID_TYPE
                     ? "there is nothing here to say what 'null' is a pointer to"
                     : "expected " + name_of(expected) + ", found 'null'");

    return INVALID_TYPE;
}

u32 ExpressionTyper::sequence(u32 scope, u32 node, u32 expected, bool array) {
    TypeTable* types = module->get_types();
    u32 wanted = INVALID_TYPE;
    u32 count = 0;

    // the context decides what it holds when it says so, and otherwise the
    // first element does and every one after has to be it -- record 0018 has
    // nothing that would make two different types one
    if (expected != INVALID_TYPE) {
        Type* entry = types->get_type(expected);

        if ((array && entry->kind == TYPE_ARRAY)
            || (!array && entry->kind == TYPE_LIST)) {
            wanted = types->get_argument(entry->first_argument);
        }
    }

    for (u32 child = first_child(node); child != 0;
         child = module->get_ast()->get_node(child)->get_sibling()) {
        u32 one = type_of(index, scope, child, wanted);

        count++;

        if (one == INVALID_TYPE) {
            return INVALID_TYPE;
        }

        if (wanted == INVALID_TYPE) {
            wanted = one;
            continue;
        }

        if (one != wanted) {
            report(child, "expected " + name_of(wanted) + ", found "
                              + name_of(one));

            return INVALID_TYPE;
        }
    }

    // written empty with nothing asking for it, there is no answer to give
    if (wanted == INVALID_TYPE) {
        report(node, "there is nothing here to say what this is empty of");

        return INVALID_TYPE;
    }

    // an array literal's length is written by how many were written, which is
    // what record 0016 keeps in the type itself
    return array ? types->array(wanted, count) : types->list(wanted);
}

u32 ExpressionTyper::tuple(u32 scope, u32 node, u32 expected) {
    TypeTable* types = module->get_types();
    std::vector<u32> wanted;
    std::vector<u32> elements;
    u32 at = 0;

    // a tuple of the same arity hands each of its own down, one per element,
    // which is what makes 'let p : (u8, f64) = (200, 1.5)' two literals that
    // took a type rather than two that were converted
    if (expected != INVALID_TYPE
        && types->get_type(expected)->kind == TYPE_TUPLE) {
        wanted = types->get_arguments(expected);
    }

    for (u32 child = first_child(node); child != 0;
         child = module->get_ast()->get_node(child)->get_sibling(), at++) {
        u32 one = type_of(index, scope, child,
                          at < wanted.size() ? wanted[at] : INVALID_TYPE);

        if (one == INVALID_TYPE) {
            return INVALID_TYPE;
        }

        elements.push_back(one);
    }

    return elements.size() == 0 ? INVALID_TYPE : types->tuple(elements);
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

    // record 0019: which declaration this written name meant. One candidate,
    // so there is nothing to choose and the answer is already the answer
    module->get_resolutions()->set_declaration(node, found[0].module,
                                               found[0].candidate);

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

u32 ExpressionTyper::logical(u32 scope, u32 node, bool unary) {
    bool ok = boolean_operand(scope, first_child(node), node);

    // the second operand is asked even when the first was wrong, so a line
    // with two mistakes on it is not read twice. 'ok' is on the right of the
    // '&&' for exactly that reason
    if (!unary) {
        ok = boolean_operand(scope, second_child(node), node) && ok;
    }

    return ok ? module->get_types()->builtin(BUILTIN_BOOL) : INVALID_TYPE;
}

bool ExpressionTyper::boolean_operand(u32 scope, u32 node, u32 at) {
    u32 wanted = module->get_types()->builtin(BUILTIN_BOOL);
    u32 given = type_of(index, scope, node, wanted);

    if (given == wanted) {
        return true;
    }

    // nothing came back and whatever could not type it has already said so --
    // a literal asked to be a bool among them
    if (given != INVALID_TYPE) {
        report(node, "'" + text_of(at) + "' needs bool, and this is " +
               name_of(given));
    }

    return false;
}

u32 ExpressionTyper::name_of_callee(u32 node) {
    // 'make<i32>()' hangs the name under an AST_GENERIC_NAME
    if (kind_of(node) == AST_GENERIC_NAME) {
        node = first_child(node);
    }

    // a dot and an arrow carry no text of their own, so the name being called
    // is the right side
    if (kind_of(node) == AST_DOT || kind_of(node) == AST_ARROW) {
        return second_child(node);
    }

    // '::' carries none either. One child is '::name' and two are
    // 'alias::name', so the name is the last of them either way
    if (kind_of(node) == AST_SCOPE) {
        u32 first = first_child(node);
        u32 second = module->get_ast()->get_node(first)->get_sibling();

        return second == 0 ? first : second;
    }

    return node;
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
    u32 at = name_of_callee(callee);

    if (chosen.status == OVERLOAD_FOUND) {
        // record 0019: *which* overload this call meant. Nothing can work it
        // out again later -- it was the argument types that picked it, and a
        // second lookup only gets the set back
        module->get_resolutions()->set_declaration(at, chosen.module,
                                                   chosen.candidate);

        // and now the literals. They came in untyped so that each candidate
        // could ask them to be its own parameter, so this is the first moment
        // any of them has a type at all
        for (u32 i = 0;
             i < arguments.size() && i < chosen.parameters.size(); i++) {
            if (arguments[i].literal) {
                module->get_resolutions()->set_type(arguments[i].node,
                                                    chosen.parameters[i]);
            }
        }
    }

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

    // recorded at the NAME and not at the dot: the dot is an operator and the
    // thing that names a declaration is its right side
    module->get_resolutions()->set_declaration(name, found[0].module,
                                               found[0].candidate);

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
    TypeTable* types = module->get_types();
    Type* entry = types->get_type(type);
    std::vector<u32> arguments = types->get_arguments(type);
    AstQuery query;
    std::string out;

    // written back the way the source writes it, because that is the only
    // spelling a reader of the diagnostic would recognise. A kind with no
    // case here would print its index, which says nothing at all
    switch ((TypeKind) entry->kind) {
    case TYPE_BUILTIN:
        return entry->subject < BUILTIN_COUNT ? BUILTIN_NAMES[entry->subject]
                                              : "?";

    case TYPE_POINTER:
        return name_of(arguments[0]) + "*";

    case TYPE_REFERENCE:
        return name_of(arguments[0]) + "&";

    case TYPE_ARRAY:
        return name_of(arguments[0]) + "["
             + (entry->subject == NO_LENGTH ? ""
                                            : std::to_string(entry->subject))
             + "]";

    case TYPE_LIST:
        return "[" + name_of(arguments[0]) + "]";

    case TYPE_HASH:
        return "{" + name_of(arguments[0]) + ": " + name_of(arguments[1])
             + "}";

    case TYPE_TUPLE:
        for (u32 i = 0; i < arguments.size(); i++) {
            out += (i > 0 ? ", " : "") + name_of(arguments[i]);
        }

        return "(" + out + ")";

    // the return is the last one, per record 0016, and it reads as the arrow
    // chain the source would write
    case TYPE_FUNCTION:
        for (u32 i = 0; i < arguments.size(); i++) {
            out += (i > 0 ? " -> " : "") + name_of(arguments[i]);
        }

        return out;

    case TYPE_GENERIC:
        return declaration_name(entry->module, entry->subject);

    case TYPE_NAMED:
        out = declaration_name(entry->module, entry->subject);

        if (arguments.size() == 0) {
            return out;
        }

        for (u32 i = 0; i < arguments.size(); i++) {
            out += (i == 0 ? "<" : ", ") + name_of(arguments[i]);
        }

        return out + ">";

    default:
        break;
    }

    return "<none>";
}

std::string ExpressionTyper::declaration_name(u32 owner, u32 candidate) {
    Module* holder = compilation->get_module(owner);
    Candidate* found = holder->get_symbols()->get_candidate(candidate);
    AstQuery query;

    // a generic parameter is its own identifier and wraps nothing
    if (found->kind == SYMBOL_GENERIC) {
        return std::string(holder->get_token_value(
            holder->get_ast()->get_node(found->ast_node)->get_token()));
    }

    query.set_module(holder);

    return query.get_declaration_name(found->ast_node);
}
