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
}

void TypeCollector::collect(u32 index) {
    walk(index, false);
}

void TypeCollector::infer(u32 index) {
    walk(index, true);
}

void TypeCollector::walk(u32 index, bool given) {
    this->index = index;
    module = compilation->get_module(index);

    SymbolTable* table = module->get_symbols();

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
                 candidate = table->get_candidate(candidate)->next_candidate) {
                table->set_candidate_type(candidate,
                                          type_of(candidate, scope, given));

                if (!given) {
                    table->set_candidate_super(candidate,
                                               super_of(candidate, scope));
                }
            }
        }
    }
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

    case SYMBOL_GENERIC:
        return module->get_types()->generic(index, candidate);

    // a field, a parameter or a variable is whatever it was written as. A
    // 'let' with no type waits for inference, which does not exist
    default:
        break;
    }

    if (!given) {
        return builder.build(index, scope,
                             query.get_written_type(found->ast_node));
    }

    return written_or_inferred(found->ast_node, scope);
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

u32 TypeCollector::written_or_inferred(u32 node, u32 scope) {
    AstQuery query;
    u32 written;
    u32 expression;

    query.set_module(module);
    written = builder.build(index, scope, query.get_written_type(node));
    expression = query.get_binding_expression(node);

    if (expression == 0) {
        return written;
    }

    // the written type is handed down, so a literal takes it rather than being
    // converted into it. With nothing written the expression decides, which is
    // the whole of inference for a binding
    u32 given = typer.type_of(index, scope, expression, written);

    if (written == INVALID_TYPE) {
        return given;
    }

    // record 0018 has no conversion, so this is equality. A literal already
    // took the written type or complained about the value, so what is left
    // here is a mismatch between two real types
    if (given != INVALID_TYPE && given != written) {
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
