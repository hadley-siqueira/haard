#include <haard/string_table/string_table.h>
#include <haard/type_table/expression_typer.h>
#include <haard/type_table/type_collector.h>

using namespace haard;

static const char* BUILTIN_NAMES[] = {
    "u8", "u16", "u32", "u64", "i8", "i16", "i32", "i64",
    "isize", "usize",
    "f32", "f64", "bool", "void", "char", "symbol"
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

    // Record 0050: the size of a pointer, which Haard assumes is 64 bits --
    // the same assumption record 0049's 'holds_a_pointer' makes, and the
    // same line to change on the day it emits for something narrower
    case BUILTIN_ISIZE: return 0x7fffffffffffffffULL;
    case BUILTIN_USIZE: return 0xffffffffffffffffULL;

    default: break;
    }

    return 0;
}

ExpressionTyper::ExpressionTyper() {
    compilation = nullptr;
    collector = nullptr;
    module = nullptr;
    index = 0;
}

void ExpressionTyper::set_collector(TypeCollector* collector) {
    this->collector = collector;

    builder.set_collector(collector);
}

void ExpressionTyper::set_compilation(Compilation* compilation) {
    this->compilation = compilation;

    resolver.set_compilation(compilation);
    overloads.set_compilation(compilation);
    builder.set_compilation(compilation);
    coercion.set_compilation(compilation);
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

    // Record 0041. A symbol is an interned name and its own type: two of them
    // compare by identity, and neither one compares with a 'char*' at all --
    // record 0018 has no conversion, so ':foo == "foo"' is a mistake about
    // types and says so.
    //
    // It takes nothing from its context. A symbol IS what it is written as,
    // which is the one literal in the language that has no other reading
    case AST_SYMBOL_LITERAL:
        return module->get_types()->builtin(BUILTIN_SYMBOL);

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

    // Bits. The same shape as arithmetic and one rule more: they are about the
    // representation, so a float has none to speak of and is refused here
    // rather than by a C++ compiler about a line nobody wrote.
    //
    // Nothing typed any of the six until 2026-09-05: 'a & b' passed the check
    // phase in silence and only the emitter noticed, by refusing to name
    // something that had no type. Agenda 2.10 counted fourteen kinds on
    // 2026-09-02 and these were not among them
    case AST_BITWISE_AND:
    case AST_BITWISE_OR:
    case AST_BITWISE_XOR:
    case AST_BITWISE_LEFT_SHIFT:
    case AST_BITWISE_RIGHT_SHIFT:
    case AST_BITWISE_UNSIGNED_RIGHT_SHIFT:
        return bitwise(scope, node, expected);

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

    case AST_INCLUSIVE_RANGE:
    case AST_EXCLUSIVE_RANGE:
        return range(scope, node);

    case AST_NEW:
        return allocation(scope, node);

    // a size is a size whatever was measured, and the operand is typed for
    // the sake of what the recording keeps rather than for an answer
    case AST_SIZEOF:
        type_of(index, scope, first_child(node), INVALID_TYPE);

        // Record 0050: a size is a 'usize', which is what that builtin is for.
        // It gave back a 'u64' until 2026-09-09, which is the same width
        // everywhere Haard emits for and the wrong answer the day it is not
        return module->get_types()->builtin(BUILTIN_USIZE);

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

    // Record 0022: a string literal is a 'char*' when nothing asks. 'char' is
    // a builtin and a pointer to one needs nothing from the standard library,
    // which is why this types before any of the standard library exists -- and
    // it is what makes 'write(char*)' next to a 'write(String)' still take the
    // first.
    //
    // Hadley, 2026-09-02: try char* first, and **become a String** when that
    // is not possible. The second half arrived on 2026-09-08 and is record
    // 0037's mechanism: where a class is asked for, the literal is a
    // CONSTRUCTION -- this typer picks the 'init' that takes what the literal
    // already is, writes it on the literal for the emitter to call, and what
    // the literal is from then on is that class.
    //
    // No name is involved. 'String' is a class with an 'init(char*)' and so is
    // any other; record 0023's named entry is what a 'char*' VALUE still needs
    case AST_STRING_LITERAL: {
        u32 own = module->get_types()->pointer(
            module->get_types()->builtin(BUILTIN_CHAR));

        if (expected == INVALID_TYPE
            || module->get_types()->get_type(expected)->kind != TYPE_NAMED) {
            return own;
        }

        u32 made = constructed_by_one(node, expected, own);

        return made == INVALID_TYPE ? own : made;
    }

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

u32 ExpressionTyper::argument_of_instantiation(u32 type) {
    Type* entry = module->get_types()->get_type(type);

    if (entry->kind != TYPE_NAMED) {
        return INVALID_TYPE;
    }

    const Instantiation* made =
        compilation->get_module(entry->module)->get_instantiation(
            entry->subject);

    if (made == nullptr || made->arguments.size() != 1) {
        return INVALID_TYPE;
    }

    // the arguments belong to the table of the module that HOLDS the clone,
    // which is record 0016's rule and the bug this project keeps making
    return builder.translate(index, entry->module, made->arguments[0]);
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

u32 ExpressionTyper::overloaded(u32 scope, u32 node, u32 left, u32 right,
                                bool quiet) {
    const char* wanted = operator_name(kind_of(node));

    if (wanted == nullptr) {
        return INVALID_TYPE;
    }

    u32 owner = index;
    std::vector<Candidacy> candidates = members_named(left, wanted, owner);

    if (candidates.size() == 0) {
        return INVALID_TYPE;
    }

    std::vector<Argument> arguments;

    if (right != 0) {
        Argument argument;
        AstNodeKind kind = kind_of(right);

        // record 0018 again: a written number has no type of its own, so it
        // is carried untyped and each candidate asks it to be its parameter,
        // and neither has 'null' until a pointer is asked for
        argument.literal = kind == AST_INTEGER_LITERAL
                        || kind == AST_FLOAT_LITERAL
                        || kind == AST_NULL_LITERAL;
        argument.node = right;
        argument.type = argument.literal
                            ? INVALID_TYPE
                            : type_of(index, scope, right, INVALID_TYPE);

        arguments.push_back(argument);
    }

    Overload chosen = overloads.choose(index, candidates, arguments);

    if (chosen.status != OVERLOAD_FOUND) {
        if (!quiet) {
            report(node, chosen.status == OVERLOAD_AMBIGUOUS
                             ? std::string("this matches more than one '")
                                   + wanted + "' equally well"
                             : std::string("no '") + wanted
                                   + "' takes these operands");
        }

        return INVALID_TYPE;
    }

    // on the operator node itself, which is where the emitter looks for it
    module->get_resolutions()->set_declaration(node, chosen.module,
                                               chosen.candidate);

    for (u32 i = 0; i < arguments.size() && i < chosen.parameters.size(); i++) {
        if (arguments[i].literal) {
            module->get_resolutions()->set_type(arguments[i].node,
                                                chosen.parameters[i]);
        }

        // Record 0037, and here it is not a nicety: 'binary' types the right
        // operand against the LEFT before this runs, so a string literal has
        // already been made a construction of the class on the left -- and
        // the ranking above then typed it a 'char*' again to rank it. Written
        // down once more, against the parameter that won, the type and the
        // constructor on that literal agree again.
        //
        // Through the reference, because an 'operator==' takes a 'String&'
        // and what that takes is a String
        if (kind_of(arguments[i].node) == AST_STRING_LITERAL) {
            type_of(index, scope, arguments[i].node,
                    module->get_types()->value_of(chosen.parameters[i]));
        }
    }

    return chosen.result;
}

u32 ExpressionTyper::subscript(u32 scope, u32 node) {
    u32 left = type_of(index, scope, first_child(node), INVALID_TYPE);

    if (left == INVALID_TYPE) {
        return INVALID_TYPE;
    }

    TypeTable* types = module->get_types();

    left = types->value_of(left);

    Type* entry = types->get_type(left);
    u32 element = element_of(left);

    if (element == INVALID_TYPE) {
        // record 0034: a class says what '[]' means on it by declaring one.
        // Asked only once the builtin shapes have had their turn, so nothing
        // a pointer or an array does can be taken over by a method
        u32 overload = overloaded(scope, node, left, second_child(node));

        if (overload != INVALID_TYPE) {
            return overload;
        }

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

    if (inner == INVALID_TYPE) {
        return INVALID_TYPE;
    }

    // Record 0035, and the one place it had not been applied: **a reference
    // is the thing it names**, so the address of one is the address of that
    // thing and not of a reference. '&xs[0]' over an Array is a 'T*' -- it
    // used to be a 'T&*', a type nothing in the language can take, and it is
    // the idiom every table in this compiler is written with: a method that
    // hands out a pointer into its own vector.
    //
    // C++ needs nothing for it: '&' on a 'T&' already gives a 'T*' there, so
    // only the type was wrong
    return module->get_types()->pointer(module->get_types()->value_of(inner));
}

u32 ExpressionTyper::dereference(u32 scope, u32 node) {
    u32 inner = type_of(index, scope, first_child(node), INVALID_TYPE);

    if (inner == INVALID_TYPE) {
        return INVALID_TYPE;
    }

    TypeTable* types = module->get_types();

    // Record 0035, and the eighth place: **a reference is the thing it
    // names**, so what a '*' asks of a 'T*&' is what it asks of a 'T*'. A
    // capture of a variant that carries a pointer is one, and so is any
    // binding of one -- found by writing a tree
    Type* entry = types->get_type(types->value_of(inner));

    if (entry->kind != TYPE_POINTER) {
        report(node, "'*' needs a pointer, and this is " + name_of(inner));

        return INVALID_TYPE;
    }

    return types->get_argument(entry->first_argument);
}

u32 ExpressionTyper::cast(u32 scope, u32 node) {
    // typed for the recording's sake and not for the answer: what a cast is,
    // is what it was written as
    u32 from = type_of(index, scope, first_child(node), INVALID_TYPE);
    u32 to = builder.build(index, scope, second_child(node));

    // Record 0049. Until 2026-09-09 this function ended one line above and
    // 'as' meant C's cast: it accepted every pair written, and the backstop
    // was g++, reporting in mangled names about a line nobody wrote. What is
    // checked here is a closed list, and a pair that is not on it is refused
    // by name -- which is not the compiler disapproving (record 0047) but the
    // compiler saying there is no such cast
    // Pointed at the 'as' and not at the type written after it: a composite
    // type node carries token 0, so a caret aimed at a named type lands on
    // the first word of the file. The parser hangs the 'as' on the cast node
    // itself, and that is a real token every time
    if (from != INVALID_TYPE && to != INVALID_TYPE && !may_cast(from, to)) {
        report(node, "there is no cast from " + name_of(from) + " to "
               + name_of(to));

        return INVALID_TYPE;
    }

    return to;
}

// The list, and it fits in a sentence: a number to a number, a pointer to a
// pointer, a pointer and a whole number either way, up or down a chain of
// bases through a pointer or a reference, and a symbol to a 'char*'.
//
// It was written from what the whole repository actually casts -- 21 distinct
// pairs, measured -- plus the pointer work a compiler written in Haard needs
// and no case had yet written. Nothing that compiled before this stopped
bool ExpressionTyper::may_cast(u32 from, u32 to) {
    TypeTable* types = module->get_types();

    // the same type written out. Pointless, and pointless is not wrong
    if (from == to) {
        return true;
    }

    Type* source = types->get_type(from);
    Type* target = types->get_type(to);

    // Record 0002 again: inside a generic nobody instantiated, 'i as T' is
    // not a program yet -- T is a parameter and not a type, so no list can
    // answer about it. The clone asks the same question with T bound, and
    // reports it there. Without this, 'std/range.hd' could not write the one
    // cast it needs
    if (source->kind == TYPE_GENERIC || target->kind == TYPE_GENERIC) {
        return true;
    }

    // A number to a number, in either direction and losing whatever it
    // loses. Record 0048 keeps every one of these OFF the implicit list, so
    // this is the only way across and it is written every time
    if (is_a_number(from) && is_a_number(to)) {
        return true;
    }

    bool from_pointer = source->kind == TYPE_POINTER;
    bool to_pointer = target->kind == TYPE_POINTER;

    // A pointer to a pointer: reading the same address as something else,
    // which is C's answer and what a compiler's own memory needs
    if (from_pointer && to_pointer) {
        return true;
    }

    // A pointer and a whole number, either way, and the number has to be one
    // a pointer FITS IN. 'p as i32' was on this list for an afternoon and it
    // is the hole this record exists to close, one level in: it passed 'hdc'
    // and g++ answered *cast from 'char*' to 'int32_t' loses precision*.
    //
    // Sixty-four bits, which Haard has nowhere to write down: there is no
    // target model, no word size, and the emitter names its integers after
    // <cstdint> and lets the C++ compiler place them. So this is an
    // assumption and not a fact, and it is the first line to change on the
    // day Haard is asked to emit for something narrower
    if ((from_pointer && holds_a_pointer(to))
        || (to_pointer && holds_a_pointer(from))) {
        return true;
    }

    // Record 0041: a symbol is a name and the emitter's table holds its text,
    // so this is the one way across and it is one way only
    if (source->kind == TYPE_BUILTIN && source->subject == BUILTIN_SYMBOL
        && to_pointer
        && types->get_type(types->get_argument(target->first_argument))
                   ->subject == BUILTIN_CHAR) {
        return true;
    }

    return one_derives_from_the_other(from, to);
}

bool ExpressionTyper::holds_a_pointer(u32 type) {
    Type* entry = module->get_types()->get_type(type);

    // 'size' and 'usize' are this question's own answer -- record 0050 makes
    // them the width of a pointer by definition, which is what they are for.
    // 'i64' and 'u64' are here because sixty-four is what Haard assumes, and
    // they were on the list before the pair existed
    return entry->kind == TYPE_BUILTIN
        && (entry->subject == BUILTIN_ISIZE || entry->subject == BUILTIN_USIZE
            || entry->subject == BUILTIN_I64
            || entry->subject == BUILTIN_U64);
}

bool ExpressionTyper::is_a_number(u32 type) {
    Type* entry = module->get_types()->get_type(type);

    if (entry->kind != TYPE_BUILTIN) {
        return false;
    }

    switch ((BuiltinType) entry->subject) {
    case BUILTIN_U8: case BUILTIN_U16: case BUILTIN_U32: case BUILTIN_U64:
    case BUILTIN_I8: case BUILTIN_I16: case BUILTIN_I32: case BUILTIN_I64:
    case BUILTIN_ISIZE: case BUILTIN_USIZE:
    case BUILTIN_F32: case BUILTIN_F64:
    case BUILTIN_BOOL: case BUILTIN_CHAR:
        return true;

    default:
        break;
    }

    // 'void' holds nothing and 'symbol' is a name, not a number
    return false;
}

// Through however many pointers and references, and the same count on both
// sides: a 'Circle**' is a 'Shape**' by the same argument a 'Circle*' is a
// 'Shape*', and a 'Circle' by value is neither -- casting a value would copy
// the base part and discard the rest, which record 0018 calls slicing and
// refuses everywhere else
bool ExpressionTyper::one_derives_from_the_other(u32 from, u32 to) {
    TypeTable* types = module->get_types();
    u32 depth = 0;

    while (true) {
        Type* source = types->get_type(from);
        Type* target = types->get_type(to);
        bool source_wraps = source->kind == TYPE_POINTER
                         || source->kind == TYPE_REFERENCE;
        bool target_wraps = target->kind == TYPE_POINTER
                         || target->kind == TYPE_REFERENCE;

        if (!source_wraps || !target_wraps) {
            break;
        }

        from = types->get_argument(source->first_argument);
        to = types->get_argument(target->first_argument);
        depth++;
    }

    // a class by value is not reached this way, which is the slicing above
    if (depth == 0) {
        return false;
    }

    return coercion.climb(index, from, to) >= 0
        || coercion.climb(index, to, from) >= 0;
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
    if (made == INVALID_TYPE) {
        return INVALID_TYPE;
    }

    TypeTable* types = module->get_types();
    u32 written = first_child(node);

    // The length of 'new T[n]' is an ordinary expression and is typed here,
    // which is the only place it can be. It sits under a **type** node, and a
    // type resolves no names and types no expressions -- TypeBuilder reads it
    // only far enough to see whether it is a literal, because a type is a
    // value and a node is not one.
    //
    // Nothing typed it until 2026-09-05, and the emitter found it the way it
    // has found every one of these: by refusing to name something the type
    // phase had never looked at
    if (kind_of(written) == AST_ARRAY_TYPE) {
        u32 length = second_child(written);
        u32 counted = length == 0
                          ? INVALID_TYPE
                          : type_of(index, scope, length, INVALID_TYPE);

        if (counted != INVALID_TYPE) {
            Type* entry = types->get_type(counted);

            // how many of something is a whole number of them. An f64 or a
            // class here is a mistake, and C++ would say so about a line
            // nobody wrote
            if (entry->kind != TYPE_BUILTIN
        || !is_a_whole_number((BuiltinType) entry->subject)) {
                report(length, "a length is a whole number, and this is "
                       + name_of(counted));
            }
        }
    }

    // Hadley, 2026-09-05: 'new T[n]' is a 'T*', which is C++'s answer and the
    // only one the rest of the language leaves open. 'T[]' is already sugar
    // for 'Array<T>' (record 0022), and 'Array<T>' is the class you *build*
    // with this, so it cannot also be what this gives back.
    //
    // The length is not in the answer, and that is the whole cost: a 'T*' does
    // not say whether it came from 'new' or from 'new[]', so which of 'delete'
    // and 'delete[]' to write is the author's to know. C++ pays the same
    // price, and record 0023 already had 'String' carrying its own 'capacity'
    // -- which only makes sense if the pointer does not carry it
    if (types->get_type(made)->kind == TYPE_ARRAY) {
        return types->pointer(
            types->get_argument(types->get_type(made)->first_argument));
    }

    return types->pointer(made);
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
        // Record 0045. A clone instantiated in the module being walked has
        // no signatures yet -- its turn comes on a later round of the same
        // walk, and catch_up steps aside for exactly that reason. So an
        // 'init' with no signature is asked for here, before it is ranked:
        // without this 'new Box<i32>(7)' was *no 'init' of Box<i32> takes
        // these arguments* about an 'init' that takes precisely those, and
        // it had been so since generics landed
        if (collector != nullptr
            && table->get_candidate(candidate)->type == INVALID_TYPE) {
            collector->type_signature_now(owner, candidate);
        }

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

        // Record 0037 made a bracket literal an 'Array<T>', so a written
        // 'Array<T>' -- or an 'f64[]', which is the same thing -- is the
        // context saying what the elements are. Hadley, 2026-09-06: infer it
        // where it is trivial and report where it is not, without trying hard.
        //
        // This is what makes 'let a : f64[]' then 'a = []' mean something
        if (!array && entry->kind == TYPE_NAMED) {
            wanted = argument_of_instantiation(expected);
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

    // Written empty with nothing asking for it. Hadley, 2026-09-06: infer
    // from the context where that is trivial, and report where it is not
    // rather than trying hard -- so this is the whole of the failure, and it
    // names what would have fixed it
    if (wanted == INVALID_TYPE) {
        report(node, "nothing here says what this is empty of, so write the "
               "type it is being given to");

        return INVALID_TYPE;
    }

    // A braced literal is a **fixed** array and its length is written by how
    // many were written, which is what record 0016 keeps in the type itself.
    // It is the primitive: a C++ array, no class, nothing to construct
    if (array) {
        u32 own = types->array(wanted, count);
        u32 made = constructed_from(scope, node, expected, own, wanted, count);

        return made == INVALID_TYPE ? own : made;
    }

    // and a bracketed one is a **dynamic** array, which record 0022 names
    // 'Array<T>' (record 0021). Built here and not in the sugar pass, because
    // the element type is exactly what a pass running before the type phase
    // cannot know
    u32 made = builder.build_generic(index, scope, node, "Array", {wanted});

    if (made == INVALID_TYPE) {
        report(node, "an array literal is an Array<T>, and 'Array' names "
               "nothing here");

        return INVALID_TYPE;
    }

    // Which 'init' builds one out of the fixed array it is made of is NOT
    // resolved here, and the reason is an ordering the architecture already
    // has: 'build_generic' has just cloned the declaration (record 0002) into
    // the module that wrote the generic, and a clone's candidates get their
    // signatures from a later sweep. Asked now, the count comes back zero and
    // the message blames a class that is fine.
    //
    // So the emitter asks, where every signature exists. That is one place
    // further down than a diagnostic wants to be, and it is a **library**
    // invariant rather than something a program can get wrong -- the standard
    // library's Array has this constructor or nothing works
    u32 wrapped = constructed_from(scope, node, expected, made, wanted, count);

    return wrapped == INVALID_TYPE ? made : wrapped;
}

// The one shape a **string** literal can reach, and the first the bracketed
// ones try: a constructor of one parameter, taking what the literal already
// is. It is on its own because a string literal has no count to write -- a
// class with 'init(char*, i32)' matches the two-parameter shape and there
// would be nothing to pass as the second argument
u32 ExpressionTyper::constructed_by_one(u32 node, u32 wanted, u32 own) {
    if (wanted == INVALID_TYPE || wanted == own || own == INVALID_TYPE
        || module->get_types()->get_type(wanted)->kind != TYPE_NAMED) {
        return INVALID_TYPE;
    }

    u32 owner = index;
    std::vector<Candidacy> candidates = constructors_of(wanted, owner);

    if (candidates.size() == 0) {
        return INVALID_TYPE;
    }

    std::vector<Argument> one;
    Argument whole;

    whole.literal = false;
    whole.node = node;
    whole.type = own;
    one.push_back(whole);

    Overload chosen = overloads.choose(index, candidates, one);

    if (chosen.status != OVERLOAD_FOUND) {
        return INVALID_TYPE;
    }

    // which constructor this literal means, written on the literal itself:
    // nothing can work it out again, exactly as for a call and for record
    // 0034's operators, and the emitter reads it here
    module->get_resolutions()->set_declaration(node, chosen.module,
                                               chosen.candidate);

    return wanted;
}

u32 ExpressionTyper::constructed_from(u32 scope, u32 node, u32 wanted, u32 own,
                                      u32 element, u32 count) {
    TypeTable* types = module->get_types();

    if (wanted == INVALID_TYPE || wanted == own || own == INVALID_TYPE
        || types->get_type(wanted)->kind != TYPE_NAMED) {
        return INVALID_TYPE;
    }

    // one parameter taking what the literal already is
    if (constructed_by_one(node, wanted, own) != INVALID_TYPE) {
        return wanted;
    }

    u32 owner = index;
    std::vector<Candidacy> candidates = constructors_of(wanted, owner);

    if (candidates.size() == 0) {
        return INVALID_TYPE;
    }

    // and two taking a pointer to its element and how many, which is the pair
    // a '{}' offers and the one Hadley named
    std::vector<Argument> two;
    Argument from;
    Argument size;

    from.literal = false;
    from.node = node;
    from.type = types->pointer(element);

    size.literal = false;
    size.node = node;
    size.type = types->builtin(BUILTIN_I32);

    two.push_back(from);
    two.push_back(size);

    Overload chosen = overloads.choose(index, candidates, two);

    if (chosen.status != OVERLOAD_FOUND) {
        return INVALID_TYPE;
    }

    (void) count;

    // which constructor this literal means, on the literal itself -- nothing
    // can work it out again, exactly as for a call and for record 0034's
    // operators
    module->get_resolutions()->set_declaration(node, chosen.module,
                                               chosen.candidate);

    return wanted;
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

    // Record 0018: a literal has no type of its own and takes the one asked
    // for, but only inside its own family -- becoming anything else would be
    // the numeric conversion the record does not have.
    //
    // Three families and not two, which this read as two until 2026-09-05: an
    // integer literal is any integer width, a float literal is an f32 or an
    // f64, and a **char literal is a char and nothing else**. A char fell into
    // the float branch, so 'let e : f64 = 'w'' passed in silence and the
    // emitter wrote 'double e = 'w';', while 'let d : i32 = 'z'' was refused
    // in words that called it a floating point literal
    bool integer = kind_of(node) == AST_INTEGER_LITERAL;
    bool character = kind_of(node) == AST_CHAR_LITERAL;
    bool wants_integer = limit_of((BuiltinType) wanted->subject) > 0;
    bool wants_char = wanted->subject == BUILTIN_CHAR;
    bool same_family = character ? wants_char
                                 : (!wants_char && integer == wants_integer);

    if (!same_family) {
        report(node, "expected " + name_of(expected) + ", found "
               + std::string(character  ? "a character"
                             : integer  ? "an integer"
                                        : "a floating point")
               + " literal");

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

    // and translated, for the reason record 0016 gives: a bare name reaches a
    // global of an imported module, and a field of a base that lives in one
    // (record 0020), and a type index means nothing outside its own table.
    //
    // This hid until 2026-09-05 behind the one decision that makes an index
    // portable: a **builtin** holds the same index in every module, which is
    // what seeding them at fixed positions bought. So every cross-module bare
    // name anyone had written -- an inherited 'wheels : i32' -- was right by
    // construction, and one of a class type would not have been
    return builder.translate(index, found[0].module,
                             value_of_candidate(found[0].module,
                                                found[0].candidate));
}

// The type a candidate has **as a value**, which is its own except for one
// shape: a variant whose payload has a default is a constructor that may be
// called with nothing, so writing its name alone is a value of the enum and
// not the constructor itself.
//
// A variant with no default keeps the signature, and giving it a name without
// calling it is a constructor as a first-class function -- which nothing has
// decided and the emitter refuses
u32 ExpressionTyper::value_of_candidate(u32 owner, u32 candidate) {
    Module* holder = compilation->get_module(owner);
    Candidate* found = holder->get_symbols()->get_candidate(candidate);
    AstQuery query;

    if ((SymbolKind) found->kind != SYMBOL_VARIANT
        || found->type == INVALID_TYPE
        || holder->get_types()->get_type(found->type)->kind != TYPE_FUNCTION) {
        return found->type;
    }

    query.set_module(holder);

    if (query.get_binding_expression(found->ast_node) == 0) {
        return found->type;
    }

    std::vector<u32> written = holder->get_types()->get_arguments(found->type);

    // the return type is the last one, per record 0016
    return written.back();
}

// whether this node is a number written down, which record 0018 gives no type
// of its own -- so it is the one thing in an operand position that would
// rather be told what to be than say
static bool is_untyped_literal(AstNodeKind kind) {
    return kind == AST_INTEGER_LITERAL || kind == AST_FLOAT_LITERAL;
}

u32 ExpressionTyper::binary(u32 scope, u32 node, u32 expected,
                            bool comparison) {
    u32 left;
    u32 right;

    // The other operand is the context, and until 2026-09-05 only one of them
    // could be: the left was typed first and handed its answer to the right,
    // so 'big - 0' worked and '0 - big' was *cannot apply this to i32 and
    // i64*. The literal on the left had nothing to learn from and took its
    // default.
    //
    // So whichever side is a written number waits for the other. Two of them
    // is the ordinary case and either order gives the same answer, which is
    // why the test is only about the left
    TypeTable* types = module->get_types();

    // What travels to the other side is the **value**, never the reference.
    // A reference is the thing it names (record 0018, amended 2026-09-06), and
    // handing 'i32&' down made the other side a literal asked to be one:
    // 'xs.at(0) + 1' was *a literal cannot be i32&*, which is a complaint
    // about the 1
    if (is_untyped_literal(kind_of(first_child(node)))
        && !is_untyped_literal(kind_of(second_child(node)))) {
        right = types->value_of(
            type_of(index, scope, second_child(node), types->value_of(expected)));
        left = type_of(index, scope, first_child(node), right);
    } else {
        left = types->value_of(
            type_of(index, scope, first_child(node), types->value_of(expected)));
        right = type_of(index, scope, second_child(node), left);
    }

    right = types->value_of(right);
    left = types->value_of(left);

    if (left == INVALID_TYPE || right == INVALID_TYPE) {
        return INVALID_TYPE;
    }

    // Record 0034: a class says what an operator means on it. Asked whenever
    // the left side is one and **not** only when the two sides differ -- two
    // Arrays are the same type, and comparing them is exactly what an
    // 'operator==' is for.
    //
    // A class that overloads nothing falls through to the rule below, which
    // is where two class values being compared with a builtin '==' is caught
    if (types->get_type(left)->kind == TYPE_NAMED) {
        // An enum is a **tag**, and two tags compare by being the same tag.
        // There is no 'operator==' to write on one and nothing to look up:
        // asking a class for its operator is right because a class is a
        // shape, and an enum is a name.
        //
        // Only '==' and '!=', because nothing has said an enum is ordered --
        // Ada says so and gives it 'Succ' and 'Pred'; this waits
        if (is_an_enum(left)
            && (kind_of(node) == AST_EQUAL || kind_of(node) == AST_NOT_EQUAL)) {
            if (left != right) {
                report(node, "cannot apply this to " + name_of(left) + " and "
                                 + name_of(right));

                return INVALID_TYPE;
            }

            return types->builtin(BUILTIN_BOOL);
        }

        const char* wanted = operator_name(kind_of(node));
        u32 owner = index;

        // A class has no arithmetic and no comparison of its own, so an
        // operator it did not overload is an error and not a fall-through.
        // It used to fall through and, when both sides were the same class,
        // pass -- 'a != b' between two Arrays typed to bool in silence and
        // came out as a C++ '!=' that does not exist. Found by an Array of
        // Arrays comparing its elements
        if (wanted != nullptr && members_named(left, wanted, owner).size() == 0) {
            report(node, name_of(left) + " has no '" + wanted + "'");

            return INVALID_TYPE;
        }

        // and when it has one, 'overloaded' either answers or has already
        // said why not
        return overloaded(scope, node, left, second_child(node));
    }

    // record 0018 has nothing that would make two different types one, so the
    // operator is where that is said
    if (left != right) {
        report(node, "cannot apply this to " + name_of(left) + " and " +
               name_of(right));

        return INVALID_TYPE;
    }

    // Record 0041: a symbol has no arithmetic. It is a pointer into a table
    // and adding two of them is a mistake the emitter would otherwise write
    // out as C++ pointer arithmetic -- the same shape as a class with no
    // operator, one builtin down
    if (!comparison && types->get_type(left)->kind == TYPE_BUILTIN
        && types->get_type(left)->subject == BUILTIN_SYMBOL) {
        report(node, "a symbol is a name and has no arithmetic");

        return INVALID_TYPE;
    }

    return comparison ? types->builtin(BUILTIN_BOOL) : left;
}

// Bits, which is arithmetic with one more rule. A float has no representation
// to operate on -- C++ refuses '1.5 & 2' too -- so this is said here, where
// the operator is, and the message names the type rather than leaving it to a
// compiler talking about generated code
u32 ExpressionTyper::bitwise(u32 scope, u32 node, u32 expected) {
    u32 result = binary(scope, node, expected, false);

    if (result == INVALID_TYPE) {
        return INVALID_TYPE;
    }

    Type* entry = module->get_types()->get_type(result);

    // BUILTIN_CHAR is a character and not a width to shift, and BUILTIN_BOOL
    // has 'and' and 'or' of its own
    if (entry->kind != TYPE_BUILTIN
        || !is_a_whole_number((BuiltinType) entry->subject)) {
        report(node, "bits are for a whole number, and this is "
               + name_of(result));

        return INVALID_TYPE;
    }

    return result;
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
    // A call written with explicit generic arguments has an AST_GENERIC_NAME
    // for a callee, and the name is its first child.
    //
    // Record 0054: the arguments used to be dropped here, which is the whole
    // of why 'f<i32>(3)' was *no 'f' takes these arguments*. The call was
    // ranked against the **unbound** signature -- 'i32 -> T -> Node*' -- and
    // an i32 does not match a T. Nothing ever instantiated a function; the
    // Instantiator was always able to, since it clones a declaration and asks
    // nothing about its kind
    u32 written = 0;

    if (kind_of(node) == AST_GENERIC_NAME) {
        written = second_child(node);
        node = first_child(node);
    }

    if (kind_of(node) == AST_IDENTIFIER) {
        return instantiated(scope, node, written,
                            resolver.resolve(index, scope, text_of(node)));
    }

    if (kind_of(node) == AST_SCOPE) {
        u32 first = first_child(node);
        u32 second = module->get_ast()->get_node(first)->get_sibling();

        if (second == 0) {
            return instantiated(scope, first, written,
                                resolver.resolve_at_module(index,
                                                           text_of(first)));
        }

        return instantiated(scope, second, written,
                            resolver.resolve_qualified(index, text_of(first),
                                                       text_of(second)));
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
    bool built = false;

    // Record 0053, and it is asked first because 'super' names nothing the
    // resolver could find: it is the base of the class this body is inside,
    // which is a question about the scope and not about a name
    if (callee != 0 && kind_of(callee) == AST_SUPER) {
        return super_call(scope, node, callee, list);
    }

    // Record 0045, and it is asked before anything is ranked: a type answers
    // to no signature, so a class among the candidates scores -1 against
    // every argument and 'String("abc")' came out as *no 'String' takes these
    // arguments* -- true, and about a question nobody had asked
    u32 made = construction(scope, node, callee, list, built);

    if (built) {
        return made;
    }

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
        // it here would make 'f(3)' pick i32 and then fail against 'f(u8)'.
        //
        // 'null' is one of them: it has no type until something says which
        // pointer it is, and at a call that something is the parameter
        argument.literal = kind == AST_INTEGER_LITERAL
                        || kind == AST_FLOAT_LITERAL
                        || kind == AST_NULL_LITERAL;
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

            // Record 0037, and this is the one place it cannot be asked
            // earlier: a string literal comes in as a 'char*' so that the
            // ranking may prefer a 'char*' parameter to a String one (record
            // 0023) -- which is a question about the CALL. Once the overload
            // is known, a class parameter takes it by a constructor this
            // typer picks, the way a written type does at a binding
            if (kind_of(arguments[i].node) == AST_STRING_LITERAL) {
                // through the reference, since what a 'String&' parameter
                // takes is a String and record 0035 makes a reference the
                // thing it names. The temporary it needs a name to bind to is
                // the emitter's problem and it already writes one
                type_of(index, scope, arguments[i].node,
                        module->get_types()->value_of(chosen.parameters[i]));
            }

            // Record 0031, and this is the fourth of the four places a value
            // is given to something. It is asked here and not in the resolver
            // because the resolver ranks and never reports: a candidate that
            // took an uncopyable class by value would simply not match, and
            // the reader would be told that no overload takes these arguments
            // -- true, and about the wrong thing
            if (!coercion.may_be_copied(index, chosen.parameters[i])) {
                report(arguments[i].node,
                       name_of(chosen.parameters[i])
                           + " cannot be copied, and this parameter takes one "
                             "by value");
            }
        }
    }

    // Record 0002: what is written inside a generic nobody instantiated is
    // not a program yet, and this is the first shape that has to say so. An
    // argument whose type is a type PARAMETER cannot be ranked against
    // anything -- 'hash_of(key)' inside a 'Hash<K, V>' matches no overload
    // until K is a type -- so the call waits for the clone, where every
    // argument is concrete and the same question is asked again and reported.
    //
    // The statement checker and the emitter already skip an unbound generic
    // whole; this is the type phase learning the same rule for the one thing
    // in it that reports
    if (chosen.status != OVERLOAD_FOUND) {
        for (const Argument& argument : arguments) {
            if (argument.type != INVALID_TYPE
                && module->get_types()->get_type(argument.type)->kind
                       == TYPE_GENERIC) {
                return INVALID_TYPE;
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

// Record 0045, and the shape it settles is that a construction is a **call
// whose callee names a type**. Nothing new is written into the tree and no
// node kind is added: 'String("abc")' parses as the call it looks like, and
// the difference is found here, by asking what the name means before anything
// is ranked.
//
// What it gives back is the type itself, and the value is a temporary the
// emitter writes in place -- 'String(p)' in C++, which is the same text the
// emitter was already writing for record 0023's conversion when it decided
// one on its own. So this works in all four places a value is given to
// something, and is not hoisted the way records 0032 and 0037 hoist: hoisting
// is exactly what keeps a bracketed literal from reaching a call, and
// reaching a call is what this is for
u32 ExpressionTyper::construction(u32 scope, u32 node, u32 callee, u32 list,
                                  bool& built) {
    built = false;

    if (callee == 0) {
        return INVALID_TYPE;
    }

    AstNodeKind kind = kind_of(callee);

    // 'i32(x)'. A builtin names a type and has no 'init' to choose, so this
    // one construction is a **conversion**, and Hadley 2026-09-09 makes it
    // the second spelling of 'x as i32' rather than a shape of its own. It is
    // the parser that lets a builtin stand where a callee goes
    if (kind == AST_BUILTIN_TYPE) {
        u32 made = builder.build(index, scope, callee);
        u32 count = 0;
        u32 from = INVALID_TYPE;
        u32 at = 0;

        built = true;

        for (u32 child = list == 0 ? 0 : first_child(list); child != 0;
             child = module->get_ast()->get_node(child)->get_sibling()) {
            // typed for the recording's sake and not for the answer, which is
            // what ExpressionTyper::cast does with the same expression
            from = type_of(index, scope, child, INVALID_TYPE);
            at = child;
            count++;
        }

        // 'i32()' and 'i32(a, b)' are neither a conversion nor anything else.
        // A cast has exactly one thing to convert
        if (count != 1) {
            report(callee, "'" + name_of(made) + "' converts one value, and "
                   + std::to_string(count) + " were written");

            return INVALID_TYPE;
        }

        // Record 0049's list, asked here as well. This spelling and 'as' are
        // the same conversion and emit the same C++, so they have to refuse
        // the same pairs -- leaving this one unchecked would have made the
        // list a suggestion, since 'i32(p)' says what 'p as i32' says
        if (from != INVALID_TYPE && made != INVALID_TYPE
            && !may_cast(from, made)) {
            report(at, "there is no cast from " + name_of(from) + " to "
                   + name_of(made));

            return INVALID_TYPE;
        }

        return made;
    }

    // the three shapes a name reaches an expression in. A '.' or a '->' is
    // not among them on purpose: 'Action.Click(1, 2)' is record 0043's
    // variant, which is callable already and must keep going down the
    // ordinary path
    if (kind != AST_IDENTIFIER && kind != AST_SCOPE
        && kind != AST_GENERIC_NAME) {
        return INVALID_TYPE;
    }

    std::vector<Candidacy> found = callee_of(scope, callee);
    u32 owner = index;
    u32 symbol = builder.type_symbol(found, owner);

    // the ordinary call, and the common one. A name that is a function, or
    // that is nothing at all, is not this record's business -- the path below
    // reports it exactly as it did before
    if (symbol == 0) {
        return INVALID_TYPE;
    }

    built = true;

    // The name and its arguments, which are the two parts an AST_NAMED_TYPE
    // holds and in the same order. So a written generic instantiates here for
    // free: 'Pair<i32, i32>(1, 2)' builds the clone before it looks for an
    // 'init', the way a written type does at a binding
    u32 name = kind == AST_GENERIC_NAME ? first_child(callee) : callee;
    u32 arguments = kind == AST_GENERIC_NAME ? second_child(callee) : 0;
    u32 made = builder.build_written_name(index, scope, name, arguments);

    if (made == INVALID_TYPE) {
        return INVALID_TYPE;
    }

    // Record 0002 again: 'T(x)' inside a generic nobody instantiated is not a
    // program yet. T is a parameter and not a class, so it has no 'init' and
    // saying so would report a mistake about a body that is fine. The clone
    // asks the same question with T bound, and reports it there
    if (module->get_types()->get_type(made)->kind == TYPE_GENERIC) {
        return made;
    }

    // Record 0043: an enum is built from one of its variants, each of which
    // is callable already. The bare name is the type and never a value, and
    // saying that it declares no 'init' would be true and useless
    if (compilation->get_module(owner)->get_symbols()
            ->get_candidate(symbol)->kind == SYMBOL_ENUM) {
        report(name, name_of(made) + " is built from one of its variants, so "
               "this needs the variant's name after a '.'");

        return INVALID_TYPE;
    }

    initialisation(scope, node, made, list);

    return made;
}

// Record 0053. Before it, a class whose every 'init' took an argument could
// not be derived from at all: the base runs before the derived's own body and
// there was nowhere to write what it takes. 'super(...)' is that place.
//
// It is not a name and does not go through the resolver. What it means is the
// base of the class this body is inside, so it is answered from the scope --
// which is also what makes it an error outside a class and outside an 'init'
// Record 0052. A range written where a value goes is a 'Range<T>', built the
// way a bracketed literal is built: the element type is what a pass before the
// type phase cannot know, so the clone is asked for here.
//
// It does not reach this function inside a 'for ... in'. Record 0040's
// lowering reads the two node kinds as **syntax** and writes a C shaped loop
// with no object at all, which is why walking a range still costs nothing
u32 ExpressionTyper::range(u32 scope, u32 node) {
    TypeTable* types = module->get_types();
    u32 first = first_child(node);
    u32 second = second_child(node);

    // whichever side is a written number waits for the other, which is what
    // 'binary' does and for the same reason: '0..big' and 'big..0' have to
    // give the same answer
    u32 left;
    u32 right;

    if (is_untyped_literal(kind_of(first))
        && !is_untyped_literal(kind_of(second))) {
        right = type_of(index, scope, second, INVALID_TYPE);
        left = type_of(index, scope, first, types->value_of(right));
    } else {
        left = type_of(index, scope, first, INVALID_TYPE);
        right = type_of(index, scope, second, types->value_of(left));
    }

    if (left == INVALID_TYPE || right == INVALID_TYPE) {
        return INVALID_TYPE;
    }

    left = types->value_of(left);
    right = types->value_of(right);

    // A range counts, so both ends are whole numbers and they are the same
    // one. Record 0048 keeps every numeric conversion off the implicit list,
    // so there is nothing here that would make two of them meet
    if (!is_a_number(left) || !is_a_number(right)
        || types->get_type(left)->subject == BUILTIN_F32
        || types->get_type(left)->subject == BUILTIN_F64) {
        report(node, "a range counts, and " + name_of(left)
               + " is not a whole number");

        return INVALID_TYPE;
    }

    if (left != right) {
        report(node, "a range's two ends are the same type, and these are "
               + name_of(left) + " and " + name_of(right));

        return INVALID_TYPE;
    }

    u32 made = builder.build_generic(index, scope, node, "Range", {left});

    if (made == INVALID_TYPE) {
        report(node, "a range is a Range<T>, and 'Range' names nothing here");

        return INVALID_TYPE;
    }

    return made;
}

u32 ExpressionTyper::super_call(u32 scope, u32 node, u32 callee, u32 list) {
    u32 inside = enclosing_class(scope);
    TypeTable* types = module->get_types();

    if (inside == 0) {
        report(callee, "'super' names the base of a class, and this is not "
               "inside one");

        return INVALID_TYPE;
    }

    u32 base = module->get_symbols()->get_candidate(inside)->super;

    if (base == INVALID_TYPE) {
        report(callee, "'super' names the base of a class, and this class "
               "derives from nothing");

        return INVALID_TYPE;
    }

    // the base's own 'init's, which is exactly what a 'new Base(...)' asks
    // for -- so the arguments are ranked, reported and recorded by record
    // 0026's own machinery and nothing here repeats it
    initialisation(scope, node, base, list);

    // a constructor gives nothing back, and 'let x = super(1)' should say so
    // rather than name a type
    return types->builtin(BUILTIN_VOID);
}

u32 ExpressionTyper::enclosing_class(u32 scope) {
    SymbolTable* table = module->get_symbols();

    while (scope != 0) {
        u32 owner = table->get_scope(scope)->owner;

        if (owner != 0) {
            u32 candidate = table->candidate_of(owner);
            SymbolKind kind = candidate == 0
                                  ? SYMBOL_NONE
                                  : (SymbolKind)
                                        table->get_candidate(candidate)->kind;

            if (kind == SYMBOL_CLASS || kind == SYMBOL_STRUCT) {
                return candidate;
            }
        }

        scope = table->get_scope(scope)->parent;
    }

    return 0;
}

std::vector<Candidacy> ExpressionTyper::instantiated(
    u32 scope, u32 at, u32 arguments,
    const std::vector<Candidacy>& found) {
    if (arguments == 0) {
        return found;
    }

    // built in THIS module's table, and translated on the way into the
    // declaring one -- record 0016, and 'instantiate_written' does it
    std::vector<u32> built;

    for (u32 child = first_child(arguments); child != 0;
         child = module->get_ast()->get_node(child)->get_sibling()) {
        built.push_back(builder.build(index, scope, child));
    }

    // Record 0002, and it is the same guard a written type has: a generic
    // naming an **unbound** parameter is not an instantiation. 'same<T>(a, b)'
    // written inside another generic would clone 'same' with T still a
    // parameter -- a declaration whose body is checked with nothing bound, and
    // reported about. The clone of the caller asks this again with T a type
    for (u32 argument : built) {
        if (argument == INVALID_TYPE
            || module->get_types()->get_type(argument)->kind == TYPE_GENERIC) {
            return found;
        }
    }

    std::vector<Candidacy> answer;
    AstQuery query;

    for (const Candidacy& candidacy : found) {
        Module* owner = compilation->get_module(candidacy.module);
        Candidate* one = owner->get_symbols()->get_candidate(candidacy.candidate);

        query.set_module(owner);

        // Only a generic FUNCTION. A generic class reaching here is record
        // 0045's construction, which took its own path before this; a
        // candidate with no type parameters is left exactly as it was, so an
        // ordinary overload of the same name still competes and the arity
        // error it would give is the one the reader wants
        if (one->kind != SYMBOL_FUNCTION
            || query.get_generic_parameters(one->ast_node).size() == 0) {
            answer.push_back(candidacy);
            continue;
        }

        u32 made = builder.instantiate_written(index, scope, at,
                                               candidacy.module,
                                               candidacy.candidate, built);

        if (made != 0) {
            Candidacy clone;

            clone.module = candidacy.module;
            clone.candidate = made;

            answer.push_back(clone);
        }
    }

    return answer;
}

std::vector<Candidacy> ExpressionTyper::members_of(u32 left, u32 name,
                                                  u32& owner) {
    return members_named(left, text_of(name), owner);
}

std::vector<Candidacy> ExpressionTyper::members_named(
    u32 left, const std::string& wanted, u32& owner) {
    std::vector<Candidacy> found;
    u32 declaration = class_of(left, owner);

    if (declaration == 0) {
        return found;
    }

    Module* holder = compilation->get_module(owner);
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
        case AST_ENUM: {
            u32 holder = table->candidate_of(owner);

            // Record 0052: a clone made mid-inference used to arrive here
            // with no type of its own, so 'this' inside its methods came out
            // as '<none>*'. The clone is typed where it is **made** now --
            // TypeBuilder asks the collector the moment the instantiator
            // hands one back -- and this reads it back
            return module->get_types()->pointer(
                holder == 0 ? INVALID_TYPE
                            : table->get_candidate(holder)->type);
        }

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

    // and **translated**, because the member may be declared in another
    // module and record 0016's rule is that a type index means nothing
    // outside the table it came from.
    //
    // It was not, until 2026-09-05. So a field whose type came from a third
    // module -- 'many : Point*' on a class of lib.holder, read from an app
    // that imports only lib.holder -- got whatever the app's own table held
    // at that number. The diagnostic told on it by naming a class of the
    // reader's module: *"Holder has no member named 'x'"* about a Point, and
    // *"F2 has no member named 'x'"* once the reader declared two structs
    return builder.translate(index, found[0].module,
                             value_of_candidate(found[0].module,
                                                found[0].candidate));
}

// Whether this named type is an enum, which is the one named type that is not
// a shape with members but a set of tags
bool ExpressionTyper::is_an_enum(u32 type) {
    Type* entry = module->get_types()->get_type(module->get_types()
                                                    ->value_of(type));

    if (entry->kind != TYPE_NAMED) {
        return false;
    }

    return compilation->get_module(entry->module)->get_symbols()
               ->get_candidate(entry->subject)->kind == SYMBOL_ENUM;
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
    return name_in(index, type);
}

// The same walk, told which table to read. An instantiation's arguments belong
// to the module that DECLARED the generic, because that is where record 0002
// puts the clone -- so rendering 'Pair<i32, f64>' crosses a module boundary
// even when the type being named did not
std::string ExpressionTyper::name_in(u32 owner, u32 type) {
    TypeTable* types = compilation->get_module(owner)->get_types();
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
        return name_in(owner, arguments[0]) + "*";

    case TYPE_REFERENCE:
        return name_in(owner, arguments[0]) + "&";

    case TYPE_ARRAY:
        return name_in(owner, arguments[0]) + "["
             + (entry->subject == NO_LENGTH ? ""
                                            : std::to_string(entry->subject))
             + "]";

    case TYPE_LIST:
        return "[" + name_in(owner, arguments[0]) + "]";

    case TYPE_HASH:
        return "{" + name_in(owner, arguments[0]) + ": " + name_in(owner, arguments[1])
             + "}";

    case TYPE_TUPLE:
        for (u32 i = 0; i < arguments.size(); i++) {
            out += (i > 0 ? ", " : "") + name_in(owner, arguments[i]);
        }

        return "(" + out + ")";

    // the return is the last one, per record 0016, and it reads as the arrow
    // chain the source would write
    case TYPE_FUNCTION:
        for (u32 i = 0; i < arguments.size(); i++) {
            out += (i > 0 ? " -> " : "") + name_in(owner, arguments[i]);
        }

        return out;

    case TYPE_GENERIC:
        return declaration_name(entry->module, entry->subject);

    case TYPE_NAMED: {
        out = declaration_name(entry->module, entry->subject);

        // A clone carries no arguments in its type -- it is a class, which is
        // the whole point of record 0002 -- so the spelling a reader would
        // recognise lives on the record that made it and nowhere else
        const Instantiation* made = compilation->get_module(entry->module)
                                        ->get_instantiation(entry->subject);

        if (made != nullptr) {
            arguments = made->arguments;
            owner = entry->module;
        }

        if (arguments.size() == 0) {
            return out;
        }

        for (u32 i = 0; i < arguments.size(); i++) {
            out += (i == 0 ? "<" : ", ") + name_in(owner, arguments[i]);
        }

        return out + ">";
    }

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
