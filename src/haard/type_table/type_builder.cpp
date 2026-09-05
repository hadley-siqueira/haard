#include <haard/type_table/type_builder.h>

using namespace haard;

static BuiltinType builtin_of(TokenKind kind) {
    switch (kind) {
    case TK_U8: return BUILTIN_U8;
    case TK_U16: return BUILTIN_U16;
    case TK_U32: return BUILTIN_U32;
    case TK_U64: return BUILTIN_U64;
    case TK_I8: return BUILTIN_I8;
    case TK_I16: return BUILTIN_I16;
    case TK_I32: return BUILTIN_I32;
    case TK_I64: return BUILTIN_I64;
    case TK_F32: return BUILTIN_F32;
    case TK_F64: return BUILTIN_F64;
    case TK_BOOL: return BUILTIN_BOOL;
    case TK_CHAR: return BUILTIN_CHAR;
    default: break;
    }

    return BUILTIN_VOID;
}

TypeBuilder::TypeBuilder() {
    compilation = nullptr;
    module = nullptr;
    index = 0;
}

void TypeBuilder::set_compilation(Compilation* compilation) {
    this->compilation = compilation;

    resolver.set_compilation(compilation);
    instantiator.set_compilation(compilation);
}

u32 TypeBuilder::build(u32 index, u32 scope, u32 node) {
    if (node == 0) {
        return INVALID_TYPE;
    }

    this->index = index;
    module = compilation->get_module(index);

    TypeTable* table = module->get_types();

    // A part that could not be built poisons what contains it, the same way
    // the parser drops a statement whose primitive failed. The check has to
    // come before the call that interns, not after it: interning first and
    // discarding the answer leaves a pointer-to-nothing in the table, which is
    // what the 'types interned' count in the goldens caught
    switch (kind_of(node)) {
    case AST_BUILTIN_TYPE: {
        Token& token = module->get_tokens()->get_token(
            module->get_ast()->get_node(node)->get_token());

        return table->builtin(builtin_of(token.get_kind()));
    }

    case AST_POINTER_TYPE: {
        u32 inner = build(index, scope, first_child(node));

        return inner == INVALID_TYPE ? INVALID_TYPE : table->pointer(inner);
    }

    case AST_REFERENCE_TYPE: {
        u32 inner = build(index, scope, first_child(node));

        return inner == INVALID_TYPE ? INVALID_TYPE : table->reference(inner);
    }

    case AST_LIST_TYPE: {
        u32 inner = build(index, scope, first_child(node));

        return inner == INVALID_TYPE ? INVALID_TYPE : table->list(inner);
    }

    case AST_ARRAY_TYPE: {
        u32 inner = build(index, scope, first_child(node));

        if (inner == INVALID_TYPE) {
            return INVALID_TYPE;
        }

        return table->array(inner, length_of(second_child(node)));
    }

    case AST_HASH_TYPE: {
        u32 key = build(index, scope, first_child(node));
        u32 value = build(index, scope, second_child(node));

        if (key == INVALID_TYPE || value == INVALID_TYPE) {
            return INVALID_TYPE;
        }

        return table->hash(key, value);
    }

    case AST_TUPLE_TYPE: {
        std::vector<u32> elements;

        for (u32 child = first_child(node); child != 0;
             child = module->get_ast()->get_node(child)->get_sibling()) {
            elements.push_back(build(index, scope, child));

            if (elements.back() == INVALID_TYPE) {
                return INVALID_TYPE;
            }
        }

        return table->tuple(elements);
    }

    // 'A -> B -> C' is written as one node with three children, so the last
    // one is the result and the others are what it takes
    case AST_FUNCTION_TYPE: {
        std::vector<u32> parameters;
        u32 result = INVALID_TYPE;

        for (u32 child = first_child(node); child != 0;
             child = module->get_ast()->get_node(child)->get_sibling()) {
            parameters.push_back(build(index, scope, child));

            if (parameters.back() == INVALID_TYPE) {
                return INVALID_TYPE;
            }
        }

        if (parameters.size() > 0) {
            result = parameters.back();
            parameters.pop_back();
        }

        return table->function(parameters, result);
    }

    case AST_NAMED_TYPE:
        return build_named(index, scope, node);

    default:
        break;
    }

    return INVALID_TYPE;
}

u32 TypeBuilder::translate(u32 into, u32 from, u32 type) {
    if (into == from || type == INVALID_TYPE) {
        return type;
    }

    TypeTable* source = compilation->get_module(from)->get_types();
    TypeTable* target = compilation->get_module(into)->get_types();
    Type* entry = source->get_type(type);

    // the whole point of seeding them at fixed indices
    if (entry->kind == TYPE_BUILTIN) {
        return type;
    }

    std::vector<u32> arguments;

    for (u32 argument : source->get_arguments(type)) {
        arguments.push_back(translate(into, from, argument));
    }

    // 'subject' is a candidate and 'module' is a module of the compilation for
    // a named or a generic type, so both carry over untouched. Only the
    // argument indices were local to the table being left
    switch ((TypeKind) entry->kind) {
    case TYPE_POINTER: return target->pointer(arguments[0]);
    case TYPE_REFERENCE: return target->reference(arguments[0]);
    case TYPE_LIST: return target->list(arguments[0]);
    case TYPE_ARRAY: return target->array(arguments[0], entry->subject);
    case TYPE_HASH: return target->hash(arguments[0], arguments[1]);
    case TYPE_TUPLE: return target->tuple(arguments);
    case TYPE_GENERIC: return target->generic(entry->module, entry->subject);

    case TYPE_FUNCTION: {
        u32 result = arguments.back();

        arguments.pop_back();

        return target->function(arguments, result);
    }

    case TYPE_NAMED:
        return target->named(entry->module, entry->subject, arguments);

    default:
        break;
    }

    return INVALID_TYPE;
}

u32 TypeBuilder::build_named(u32 index, u32 scope, u32 node) {
    u32 name = first_child(node);
    u32 arguments = second_child(node);
    std::vector<Candidacy> found;
    std::string text;

    // the name is what Parser::parse_scope read, so it is an identifier or one
    // of the two qualified forms, exactly as a use is
    if (kind_of(name) == AST_SCOPE) {
        u32 first = first_child(name);
        u32 second = module->get_ast()->get_node(first)->get_sibling();

        text = std::string(module->get_token_value(
            module->get_ast()->get_node(second == 0 ? first : second)
                ->get_token()));

        found = second == 0
                    ? resolver.resolve_at_module(index, text)
                    : resolver.resolve_qualified(
                          index,
                          std::string(module->get_token_value(
                              module->get_ast()->get_node(first)->get_token())),
                          text);
    } else {
        text = std::string(module->get_token_value(
            module->get_ast()->get_node(name)->get_token()));
        found = resolver.resolve(index, scope, text);
    }

    u32 owner = index;
    u32 symbol = type_symbol(found, owner);

    if (symbol == 0) {
        return INVALID_TYPE;
    }

    Candidate* candidate =
        compilation->get_module(owner)->get_symbols()->get_candidate(symbol);

    // An instantiation binds a parameter by setting the type of its
    // candidate, and this is the one place that reads it: inside a clone, 'A'
    // is not a stand-in for i32, it is i32. Everywhere else the parameter is
    // still itself, and its candidate holds the TYPE_GENERIC that says so --
    // so the same line answers both
    if (candidate->kind == SYMBOL_GENERIC) {
        if (candidate->type != INVALID_TYPE) {
            return translate(index, owner, candidate->type);
        }

        return module->get_types()->generic(owner, symbol);
    }

    std::vector<u32> built;

    for (u32 child = arguments == 0 ? 0 : first_child(arguments); child != 0;
         child = module->get_ast()->get_node(child)->get_sibling()) {
        built.push_back(build(index, scope, child));

        if (built.back() == INVALID_TYPE) {
            return INVALID_TYPE;
        }
    }

    AstQuery query;

    query.set_module(compilation->get_module(owner));

    // Record 0002: a generic declaration is not a type, it is something a
    // type is made from. What a use of it names is the clone, which is an
    // ordinary class -- so this is the last line of the compiler that knows
    // a generic was involved.
    //
    // Asked whenever the DECLARATION has parameters and not only when the use
    // wrote arguments, so that a bare 'Pair' is an arity error and not a type
    // whose fields are parameters nothing bound
    if (built.size() > 0 || query.get_generic_parameters(
                                candidate->ast_node).size() > 0) {
        std::vector<u32> translated;

        for (u32 argument : built) {
            translated.push_back(translate(owner, index, argument));
        }

        // the NAME and never the composite: a type node carries token 0, so
        // a caret pointing at one lands on whatever token 0 is -- the first
        // word of the file
        u32 at = name;

        if (kind_of(name) == AST_SCOPE) {
            u32 first = first_child(name);
            u32 second = module->get_ast()->get_node(first)->get_sibling();

            at = second == 0 ? first : second;
        }

        u32 made = instantiator.instantiate(index, at, owner, symbol,
                                            translated);

        if (made == 0) {
            return INVALID_TYPE;
        }

        return module->get_types()->named(owner, made, std::vector<u32>());
    }

    return module->get_types()->named(owner, symbol, built);
}

u32 TypeBuilder::type_symbol(const std::vector<Candidacy>& found, u32& owner) {
    for (const Candidacy& candidacy : found) {
        Candidate* candidate = compilation->get_module(candidacy.module)
                                   ->get_symbols()
                                   ->get_candidate(candidacy.candidate);

        switch ((SymbolKind) candidate->kind) {
        case SYMBOL_CLASS:
        case SYMBOL_STRUCT:
        case SYMBOL_ENUM:
        case SYMBOL_UNION:
        case SYMBOL_GENERIC:
            owner = candidacy.module;

            return candidacy.candidate;

        default:
            break;
        }
    }

    return 0;
}

u32 TypeBuilder::length_of(u32 node) {
    if (node == 0) {
        return NO_LENGTH;
    }

    if (kind_of(node) != AST_INTEGER_LITERAL) {
        return NO_LENGTH;
    }

    std::string text = std::string(module->get_token_value(
        module->get_ast()->get_node(node)->get_token()));
    u32 value = 0;

    for (char digit : text) {
        if (digit >= '0' && digit <= '9') {
            value = value * 10 + (u32) (digit - '0');
        }
    }

    return value;
}

u32 TypeBuilder::first_child(u32 node) {
    return module->get_ast()->get_node(node)->get_children();
}

u32 TypeBuilder::second_child(u32 node) {
    u32 first = first_child(node);

    return first == 0 ? 0 : module->get_ast()->get_node(first)->get_sibling();
}

AstNodeKind TypeBuilder::kind_of(u32 node) {
    return (AstNodeKind) module->get_ast()->get_node(node)->get_kind();
}
