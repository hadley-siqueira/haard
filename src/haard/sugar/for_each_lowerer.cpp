#include <haard/compilation/compilation.h>
#include <haard/sugar/for_each_lowerer.h>

using namespace haard;

ForEachLowerer::ForEachLowerer() {
    compilation = nullptr;
    module = nullptr;
    index = 0;
    like = 0;
    counter = 0;
}

void ForEachLowerer::set_compilation(Compilation* compilation) {
    this->compilation = compilation;

    typer.set_compilation(compilation);
}

void ForEachLowerer::set_collector(TypeCollector* collector) {
    typer.set_collector(collector);
}

u32 ForEachLowerer::lower(u32 index, u32 scope, u32 for_each, u32& written) {
    this->index = index;
    module = compilation->get_module(index);
    written = INVALID_TYPE;

    builder.set_ast(module->get_ast());
    query.set_module(module);
    symbols.set_module(module);
    typer.set_module(index);

    // one attempt per loop, whatever it ends in
    if (seen.count(std::make_pair(index, for_each)) > 0) {
        return 0;
    }

    seen.insert(std::make_pair(index, for_each));

    if (inside_an_unbound_generic(scope)) {
        return 0;
    }

    like = token_of(for_each);

    u32 head = child_of(for_each, AST_FOR_HEAD);
    u32 sequence = 0;

    for (u32 child = head == 0 ? 0 : first_child(head); child != 0;
         child = sibling_of(child)) {
        sequence = child;
    }

    // 'for x not in xs' is a foreach as far as the parser is concerned -- one
    // word, and which loop it is shows only from the last expression of the
    // head -- and it says the opposite of what a walk needs
    if (sequence == 0 || kind_of(sequence) != AST_IN) {
        report(sequence == 0 ? for_each : sequence,
               "a 'for' walks what is written after 'in'");

        return 0;
    }

    std::vector<u32> variables = query.get_loop_variables(for_each);

    // 'for key, value in pairs' is the shape the reference left unfinished
    // and record 0040 did not decide: it needs a way to take a value apart,
    // which is agenda 1.23's question and not this one's
    if (variables.size() != 1) {
        report(for_each, "a 'for ... in' binds one name, and this writes " +
                             std::to_string(variables.size()));

        return 0;
    }

    u32 variable = variables[0];
    u32 container = sibling_of(first_child(sequence));

    if (container == 0) {
        report(sequence, "a 'for' walks what is written after 'in'");

        return 0;
    }

    // and from here on a diagnostic is about the sequence, so that is what
    // the tokens this pass writes are made to look like
    like = token_of(container) == 0 ? like : token_of(container);

    // a range is syntax and not a value: '0..10' has no type to ask about,
    // and record 0040 gives it no class to have one
    if (kind_of(container) == AST_EXCLUSIVE_RANGE
        || kind_of(container) == AST_INCLUSIVE_RANGE) {
        return over_a_range(scope, for_each, variable, container, written);
    }

    u32 type = typer.type_of(index, scope, container, INVALID_TYPE);

    // whatever is wrong with the expression was reported where it was
    // written, and saying it again in this pass's words would be one mistake
    // read as two
    if (type == INVALID_TYPE) {
        return 0;
    }

    TypeTable* types = module->get_types();
    u32 value = types->value_of(type);
    Type* entry = types->get_type(value);

    // a pointer to a class is walked like the class: record 0018's '.' reads
    // a member of a T and of a T* alike, so the calls below need no case for
    // it
    if (entry->kind == TYPE_POINTER) {
        value = types->get_argument(entry->first_argument);
        entry = types->get_type(value);
    }

    if (entry->kind == TYPE_NAMED) {
        return over_a_cursor(scope, for_each, variable, container);
    }

    // The length is in the **type**, where record 0016 keeps it, so the
    // compiler has it and a fixed array is iterable. It is also the one shape
    // that could not have been lowered by a syntactic pass, and so the reason
    // this one runs where it does
    if (entry->kind == TYPE_ARRAY && entry->subject != NO_LENGTH) {
        return over_a_fixed_array(scope, for_each, variable, container,
                                  entry->subject);
    }

    report(container,
           typer.name_of(type) + " is not something a 'for ... in' can walk");

    return 0;
}

// Record 0040's first shape and the one the standard library is written for:
//
//     let __c0 = xs.iterator()
//
//     while __c0.has_next():
//         let x = __c0.next()
//
// The cursor holds the **container** and not a copy of its insides, which is
// the library's business and not this pass's -- what this pass promises is
// only that 'iterator', 'has_next' and 'next' are called by those names, and
// that they resolve like any other call. A class that answers none of them is
// reported by the call itself, which is the diagnostic a reader can act on
u32 ForEachLowerer::over_a_cursor(u32 scope, u32 for_each, u32 variable,
                                  u32 container) {
    u32 body = child_of(for_each, AST_BLOCK);
    u32 name = name_for("__c");

    // detached before it is moved: it was the right side of the 'in', and
    // what follows it there is not what follows it here
    module->get_ast()->get_node(container)->set_sibling(0);

    u32 made = call_on(container, "iterator");

    // Typed here, and written down on the local, because the loop variable is
    // typed in a moment out of '__cN.next()' -- and the round of inference
    // that would reach this declaration on its own comes after that
    u32 type = typer.type_of(index, scope, made, INVALID_TYPE);

    // The class answers no 'iterator', which the typer has just said in the
    // words a reader can act on. Nothing is built on top of that: a local
    // declared here is a candidate of its own, and the collector would reach
    // it and type this same call a **second** time -- one mistake read as
    // two. The loop is left standing, which is what every other refusal in
    // this pass does.
    //
    // It came out as one error only because 'TypeCollector::walk' skipped
    // exactly one new candidate per round, and this local was it. Record 0056
    // fixed that off-by-one and this is what it uncovered
    if (type == INVALID_TYPE) {
        // and the container goes back to being what it was. 'call_on' made
        // the synthetic 'iterator' its **sibling**, and the 'in' node still
        // holds the container as a child -- so a walk of the standing loop
        // would reach a name this pass invented and report that it names
        // nothing. The call above is left where it is, unreachable from the
        // root, which is what every node this pass builds and does not use
        // already is
        module->get_ast()->get_node(container)->set_sibling(0);

        return 0;
    }

    u32 cursor = make_local(scope, name, made, type);

    u32 token = module->add_synthetic_token(TK_WHILE, "while", like);
    u32 loop = builder.make_while(token);
    u32 last =
        builder.add_child(loop, 0, call_on(identifier_like(name), "has_next"));

    builder.add_child(loop, last, body);

    // at the top of the body and not at the end of it, so a 'continue' in the
    // body cannot skip the step that moves the cursor on
    u32 declaration =
        make_loop_binding(variable, call_on(identifier_like(name), "next"));

    prepend(body, declaration);
    rewrite_into_block(for_each, std::vector<u32>{cursor, loop});

    return first_child(declaration);
}

// The second shape, with the length read off the type:
//
//     let __i0 = 0
//
//     for ; __i0 < 3; __i0 = __i0 + 1:
//         let x = fixed[__i0]
//
// The sequence is indexed every turn rather than held in a local, because a
// fixed array in a local is a **copy** of it -- the loop would then walk a
// copy and writing through 'x' would write into it
u32 ForEachLowerer::over_a_fixed_array(u32 scope, u32 for_each, u32 variable,
                                       u32 container, u32 length) {
    u32 body = child_of(for_each, AST_BLOCK);
    u32 name = name_for("__i");

    module->get_ast()->get_node(container)->set_sibling(0);

    u32 counted = make_local(scope, name, integer("0"),
                             module->get_types()->builtin(BUILTIN_I32));
    u32 bracket =
        module->add_synthetic_token(TK_LEFT_SQUARE_BRACKET, "[", like);
    u32 element =
        builder.make_index(bracket, container, identifier_like(name));
    u32 declaration = make_loop_binding(variable, element);

    prepend(body, declaration);

    u32 loop = counted_loop(
        name, condition_of(name, integer(std::to_string(length)), false),
        body);

    rewrite_into_block(for_each, std::vector<u32>{counted, loop});

    return first_child(declaration);
}

// The third, which needs no class at all:
//
//     let __e0 = 10
//     let i = 0
//
//     for ; i < __e0; i = i + 1:
//
// The end is bound to a local because it is evaluated **once**: written into
// the condition it would run every turn, and 'for i in 0..xs.length()' would
// be a call per element
u32 ForEachLowerer::over_a_range(u32 scope, u32 for_each, u32 variable,
                                 u32 range, u32& written) {
    u32 body = child_of(for_each, AST_BLOCK);
    u32 from = first_child(range);
    u32 to = from == 0 ? 0 : sibling_of(from);

    // the parser poisons what it could not read, so this is a range with one
    // end and not a tree nobody has looked at
    if (from == 0 || to == 0) {
        report(range, "a range needs both of its ends");

        return 0;
    }

    Ast* ast = module->get_ast();

    ast->get_node(from)->set_sibling(0);
    ast->get_node(to)->set_sibling(0);

    // and it is the end that decides the type of the loop variable. Record
    // 0018 gives a written number no type of its own, so 'for i in 0..n' over
    // a u32 walks with a u32 -- while inference reading the '0' alone would
    // make i an i32 and the comparison a mistake about types nobody wrote
    u32 name = name_for("__e");

    written = typer.type_of(index, scope, to, INVALID_TYPE);

    u32 limit = make_local(scope, name, to, written);
    u32 declaration = make_loop_binding(variable, from);
    u32 loop = counted_loop(
        token_of(variable),
        condition_of(token_of(variable), identifier_like(name),
                     kind_of(range) == AST_INCLUSIVE_RANGE),
        body);

    rewrite_into_block(for_each,
                       std::vector<u32>{limit, declaration, loop});

    return first_child(declaration);
}

u32 ForEachLowerer::counted_loop(u32 name, u32 condition, u32 body) {
    u32 token = module->add_synthetic_token(TK_FOR, "for", like);
    u32 plus = module->add_synthetic_token(TK_PLUS, "+", like);
    u32 assignment = module->add_synthetic_token(TK_ASSIGNMENT, "=", like);

    u32 sum = builder.make_binary_operator(AST_PLUS, plus,
                                           identifier_like(name), integer("1"));
    u32 stepped = builder.make_binary_operator(
        AST_ASSIGNMENT, assignment, identifier_like(name), sum);

    u32 loop = builder.make_for(token);
    u32 last = builder.add_child(loop, 0, builder.make_for_head());
    u32 part = builder.make_for_condition();

    builder.add_child(part, 0, condition);
    last = builder.add_child(loop, last, part);

    part = builder.make_for_increment();

    builder.add_child(part, 0, stepped);
    last = builder.add_child(loop, last, part);

    builder.add_child(loop, last, body);

    return loop;
}

u32 ForEachLowerer::condition_of(u32 name, u32 limit, bool inclusive) {
    u32 token = module->add_synthetic_token(
        inclusive ? TK_LESS_THAN_OR_EQUAL : TK_LESS_THAN,
        inclusive ? "<=" : "<", like);

    return builder.make_binary_operator(
        inclusive ? AST_LESS_THAN_OR_EQUAL : AST_LESS_THAN, token,
        identifier_like(name), limit);
}

u32 ForEachLowerer::make_local(u32 scope, u32 name, u32 expression, u32 type) {
    u32 token = module->add_synthetic_token(TK_LET, "let", like);
    u32 binding = builder.make_binding(
        builder.make_binding_name(builder.make_identifier(name)), 0,
        builder.make_binding_expression(expression));
    u32 statement = builder.make_let_declaration(token, binding);

    // the same call record 0002's clone is declared by: a declaration made
    // after the symbol phase has run gets exactly the names that phase would
    // have given it, and nothing downstream can tell the difference
    symbols.collect_declaration(scope, statement, "");

    u32 candidate = module->get_symbols()->candidate_of(binding);

    if (candidate != 0) {
        module->get_symbols()->set_candidate_type(candidate, type);
    }

    return statement;
}

u32 ForEachLowerer::make_loop_binding(u32 variable, u32 expression) {
    u32 token = module->add_synthetic_token(TK_LET, "let", like);

    // the identifier the source wrote, moved out of the head and into the
    // binding. Reusing it is what keeps the name, the text and the position
    // the author's -- a diagnostic about the loop variable points at the
    // loop variable
    module->get_ast()->get_node(variable)->set_sibling(0);

    u32 binding =
        builder.make_binding(builder.make_binding_name(variable), 0,
                             builder.make_binding_expression(expression));

    return builder.make_let_declaration(token, binding);
}

u32 ForEachLowerer::call_on(u32 receiver, const std::string& method) {
    u32 dot = module->add_synthetic_token(TK_DOT, ".", like);
    u32 name = module->add_synthetic_token(TK_IDENTIFIER, method, like);
    u32 open = module->add_synthetic_token(TK_LEFT_PARENTHESIS, "(", like);

    u32 callee = builder.make_binary_operator(AST_DOT, dot, receiver,
                                              builder.make_identifier(name));

    return builder.make_call(open, callee, builder.make_arguments(open));
}

u32 ForEachLowerer::identifier_like(u32 token) {
    return builder.make_identifier(token);
}

u32 ForEachLowerer::integer(const std::string& digits) {
    return builder.make_literal(
        AST_INTEGER_LITERAL,
        module->add_synthetic_token(TK_INTEGER_LITERAL, digits, like));
}

u32 ForEachLowerer::name_for(const std::string& prefix) {
    return module->add_synthetic_token(TK_IDENTIFIER,
                                       prefix + std::to_string(counter++),
                                       like);
}

void ForEachLowerer::rewrite_into_block(u32 for_each,
                                        const std::vector<u32>& statements) {
    AstNode* rewritten = module->get_ast()->get_node(for_each);
    u32 last = 0;

    // Rewritten in place, and not replaced in its parent: it keeps its index
    // and its sibling link, so nothing else in the tree moves and no walk has
    // to know a parent. It also keeps the **scope** the collector opened for
    // the loop, since a scope is stamped with the node that opened it -- and
    // that scope is exactly where the loop variable and these locals belong
    rewritten->set_kind(AST_BLOCK);
    rewritten->set_children(0);

    for (u32 statement : statements) {
        last = builder.add_child(for_each, last, statement);
    }
}

void ForEachLowerer::prepend(u32 block, u32 statement) {
    Ast* ast = module->get_ast();

    ast->get_node(statement)->set_sibling(ast->get_node(block)->get_children());
    ast->get_node(block)->set_children(statement);
}

bool ForEachLowerer::inside_an_unbound_generic(u32 scope) {
    SymbolTable* table = module->get_symbols();

    for (u32 at = scope; at != 0; at = table->get_scope(at)->parent) {
        u32 owner = table->get_scope(at)->owner;

        if (owner == 0) {
            continue;
        }

        for (u32 parameter : query.get_generic_parameters(owner)) {
            u32 candidate = table->candidate_of(parameter);

            if (candidate == 0) {
                continue;
            }

            u32 type = table->get_candidate(candidate)->type;

            if (type == INVALID_TYPE
                || module->get_types()->get_type(type)->kind == TYPE_GENERIC) {
                return true;
            }
        }
    }

    return false;
}

void ForEachLowerer::report(u32 node, const std::string& message) {
    Token& token = module->get_tokens()->get_token(token_of(node));

    module->get_logger()->error(token.get_offset(), token.get_length(),
                                message);
}

AstNodeKind ForEachLowerer::kind_of(u32 node) {
    return module->get_ast()->get_node(node)->get_kind();
}

u32 ForEachLowerer::child_of(u32 node, AstNodeKind kind) {
    for (u32 child = first_child(node); child != 0; child = sibling_of(child)) {
        if (kind_of(child) == kind) {
            return child;
        }
    }

    return 0;
}

u32 ForEachLowerer::first_child(u32 node) {
    return module->get_ast()->get_node(node)->get_children();
}

u32 ForEachLowerer::sibling_of(u32 node) {
    return module->get_ast()->get_node(node)->get_sibling();
}

u32 ForEachLowerer::token_of(u32 node) {
    return module->get_ast()->get_node(node)->get_token();
}
