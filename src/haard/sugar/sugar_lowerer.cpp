#include <haard/sugar/sugar_lowerer.h>

using namespace haard;

SugarLowerer::SugarLowerer() {
    module = nullptr;
    counter = 0;
}

void SugarLowerer::set_module(Module* module) {
    this->module = module;
    builder.set_ast(module->get_ast());
    counter = 0;
}

void SugarLowerer::lower() {
    // no block and no statement yet: a template string written at module
    // level -- a global's value, a field's, a parameter's default -- has
    // nowhere to be built before, and is refused when it is reached
    walk(module->get_ast()->get_root(), 0, 0);
}

// Record 0061. Everything this pass takes apart is built **before the
// statement it is written in**, which is only the same program where the
// statement evaluates that part exactly once and before anything else that
// can decide not to. Four places do not, and each is rewritten first into a
// shape where it does -- so the hoisting itself never has to know:
//
//   - the right of 'and' and of 'or', which may not run at all
//   - the condition of a loop and a C shaped 'for''s step, which run every turn
//   - the condition of an 'elif', which runs only when those above were false
//
// Each is rewritten only when what it holds would be hoisted, so a program
// that writes no template string and no unbound literal in one of those
// places is the tree it always was
void SugarLowerer::walk(u32 node, u32 block, u32 statement) {
    if (node == 0) {
        return;
    }

    switch (kind_of(node)) {
    // where a statement is, and so where anything lifted out of one goes.
    // Each child is both the statement and the thing being walked
    case AST_BLOCK:
        for (u32 child = first_child(node); child != 0;
             child = sibling_of(child)) {
            walk(child, node, child);
        }

        return;

    case AST_WHILE:
        if (hoists(first_child(node))) {
            condition_into_the_body(node);
        }

        walk_children(node, block, statement);
        return;

    case AST_FOR:
        take_the_step_apart(node, block, statement);
        walk_children(node, block, statement);
        return;

    case AST_IF:
        elif_into_an_else(node);
        walk_children(node, block, statement);
        return;

    // The left of a short circuit is evaluated whatever happens, so it is as
    // safe as anywhere. The right is the one that may not run
    case AST_LOGICAL_AND:
    case AST_LOGICAL_OR:
        if (block != 0 && hoists(sibling_of(first_child(node)))) {
            short_circuit_into_a_branch(node, block, statement);
            return;
        }

        walk_children(node, block, statement);
        return;

    // A bracket or brace literal that is NOT bound to a name. The emitter
    // builds one where it is bound, because what it is made of is a C++ array
    // and a C++ array is a declaration -- so anywhere else it has to become a
    // binding first
    case AST_LIST:
    case AST_ARRAY:
        walk_children(node, block, statement);

        // An **empty** one is left where it stands, and for a reason that is
        // not an exception: what a literal is hoisted for is the fixed array
        // it is made of, which is a C++ declaration -- and an empty one is
        // made of nothing. Moving it would also throw away the only thing
        // that can say what it is empty of, which is the type it is being
        // given to (Hadley, 2026-09-06)
        if (first_child(node) == 0) {
            return;
        }

        if (block == 0) {
            refuse(node, "an array literal");
            return;
        }

        hoist_literal(node, block, statement);
        return;

    // and one that IS bound to a name is already where the emitter wants it,
    // so only its elements are walked
    case AST_BINDING_EXPRESSION: {
        u32 held = first_child(node);

        if (held != 0
            && (kind_of(held) == AST_LIST || kind_of(held) == AST_ARRAY)) {
            walk_children(held, block, statement);
            return;
        }

        walk_children(node, block, statement);
        return;
    }

    // 'T[]' with nothing between the brackets. 'T[3]' is a fixed array and
    // stays one (record 0021), so the length is what tells them apart
    case AST_ARRAY_TYPE:
        walk_children(node, block, statement);

        if (sibling_of(first_child(node)) == 0) {
            lower_into_generic(node, "Array");
        }

        return;

    // '[T]' and '{K: V}', the other two spellings record 0022 decided are
    // written form for a class of the standard library. They were **not**
    // lowered until 2026-09-10 and were a structural type of their own
    // instead: 'let l : [i32]' compiled and then answered to no method, and
    // 'let l : [i32] = [1, 2, 3]' was *expected [i32], found Array<i32>*.
    // 'List<i32>' worked all along, which is what made it a hole and not a
    // decision -- the sugar reached the literal and not the annotation.
    //
    // Neither has a second reading the way 'T[3]' is a second reading of
    // 'T[]', so neither asks a question before it is rewritten
    case AST_LIST_TYPE:
        walk_children(node, block, statement);
        lower_into_generic(node, "List");
        return;

    case AST_HASH_TYPE:
        walk_children(node, block, statement);
        lower_into_generic(node, "Hash");
        return;

    // its interpolations first, so that a template string written inside one
    // is already a local by the time this one appends it -- and its
    // statements, inserted before the same statement, land before these
    case AST_TEMPLATE_STRING:
        walk_children(node, block, statement);

        if (block == 0) {
            refuse(node, "a template string");
            return;
        }

        lower_template_string(node, block, statement);
        return;

    default:
        walk_children(node, block, statement);
        return;
    }
}

void SugarLowerer::walk_children(u32 node, u32 block, u32 statement) {
    u32 child = first_child(node);

    // read the sibling before walking, because lowering rewrites the node it
    // was given and a rewritten template string has no children left
    while (child != 0) {
        u32 next = sibling_of(child);

        walk(child, block, statement);
        child = next;
    }
}

// Whether anything under this node would be built before its statement. A
// block is where a closure's body starts, and what is inside it is built
// before a statement of its own
bool SugarLowerer::hoists(u32 node) {
    if (node == 0) {
        return false;
    }

    switch (kind_of(node)) {
    case AST_BLOCK:
        return false;

    case AST_TEMPLATE_STRING:
        return true;

    case AST_LIST:
    case AST_ARRAY:
        if (first_child(node) != 0) {
            return true;
        }

        break;

    default:
        break;
    }

    for (u32 child = first_child(node); child != 0; child = sibling_of(child)) {
        if (hoists(child)) {
            return true;
        }
    }

    return false;
}

// 'a and b' is the branch it always was:
//
//     let __sc0 : bool = a
//     if __sc0:
//         __sc0 = b
//
// and 'a or b' the same with 'if not __sc0'. The right is now a statement in
// a block of its own, so whatever it builds is built there -- only when it
// runs. The two new statements go before the one the operator was written in,
// and are walked as statements, so a left that builds something builds it
// before the 'let', and an 'and' on the right is taken apart inside the 'if'
void SugarLowerer::short_circuit_into_a_branch(u32 node, u32 block,
                                               u32 statement) {
    u32 like = token_of(node);
    u32 left = first_child(node);
    u32 right = sibling_of(left);
    bool is_and = kind_of(node) == AST_LOGICAL_AND;
    u32 name = module->add_synthetic_token(
        TK_IDENTIFIER, "__sc" + std::to_string(counter++), like);

    module->get_ast()->get_node(left)->set_sibling(0);

    u32 declaration = make_flag(name, left, like);

    u32 test = builder.make_identifier(name);

    if (!is_and) {
        test = builder.make_unary_operator(
            AST_LOGICAL_NOT, module->add_synthetic_token(TK_NOT, "not", like),
            test);
    }

    u32 body = builder.make_block();

    builder.add_child(body, 0, make_assignment(name, right, like));

    u32 branch = builder.make_if(module->add_synthetic_token(TK_IF, "if",
                                                             like));

    builder.add_child(branch, builder.add_child(branch, 0, test), body);

    // and the operator becomes the answer, in place
    AstNode* rewritten = module->get_ast()->get_node(node);

    rewritten->set_kind(AST_IDENTIFIER);
    rewritten->set_token(name);
    rewritten->set_children(0);

    insert_before(block, statement, std::vector<u32>{declaration, branch});

    walk(declaration, block, declaration);
    walk(branch, block, branch);
}

// 'while c:' is 'while true:' with the condition asked first thing in every
// turn, where whatever it builds is built every turn:
//
//     while true:
//         if not c:
//             break
//         ...
void SugarLowerer::condition_into_the_body(u32 node) {
    u32 condition = first_child(node);
    u32 body = sibling_of(condition);
    u32 like = token_of(node);
    u32 always = builder.make_literal(
        AST_TRUE, module->add_synthetic_token(TK_TRUE, "true", like));

    module->get_ast()->get_node(condition)->set_sibling(0);
    module->get_ast()->get_node(always)->set_sibling(body);
    module->get_ast()->get_node(node)->set_children(always);

    prepend(body, std::vector<u32>{make_exit_unless(condition, like)});
}

// A C shaped 'for' runs its condition every turn and its step after every
// turn, 'continue' included. Only the condition building something:
//
//     for a; ; c:
//         if not b:
//             break
//         ...
//
// and a 'continue' still reaches 'c' and then the condition. A step that
// builds something cannot move to the end of the body -- a 'continue' would
// skip it -- so it moves to the **top**, behind a flag the step itself sets,
// and the condition moves with it, since the condition runs after the step:
//
//     let __step0 : bool = false
//     for a; ; __step0 = true:
//         if __step0:
//             c
//         if not b:
//             break
//         ...
void SugarLowerer::take_the_step_apart(u32 node, u32 block, u32 statement) {
    u32 head = first_child(node);
    u32 condition = sibling_of(head);
    u32 step = condition == 0 ? 0 : sibling_of(condition);
    u32 body = step == 0 ? 0 : sibling_of(step);
    u32 tested = condition == 0 ? 0 : first_child(condition);
    bool moves_step = step != 0 && hoists(step);

    if (body == 0 || (!moves_step && !hoists(tested))) {
        return;
    }

    u32 like = token_of(node);
    std::vector<u32> first;

    if (moves_step) {
        u32 name = module->add_synthetic_token(
            TK_IDENTIFIER, "__step" + std::to_string(counter++), like);
        u32 steps = builder.make_block();
        u32 last = 0;

        for (u32 part = first_child(step); part != 0; ) {
            u32 next = sibling_of(part);

            module->get_ast()->get_node(part)->set_sibling(0);
            last = builder.add_child(steps, last, part);
            part = next;
        }

        module->get_ast()->get_node(step)->set_children(0);
        builder.add_child(step, 0, make_assignment(
            name, builder.make_literal(AST_TRUE, module->add_synthetic_token(
                                                     TK_TRUE, "true", like)),
            like));

        u32 branch = builder.make_if(module->add_synthetic_token(TK_IF, "if",
                                                                 like));

        builder.add_child(branch,
                          builder.add_child(branch, 0,
                                            builder.make_identifier(name)),
                          steps);
        first.push_back(branch);

        // the flag is declared where the 'for' is, so it starts false every
        // time the loop is reached and not once per program
        insert_before(block, statement, std::vector<u32>{make_flag(
            name,
            builder.make_literal(AST_FALSE, module->add_synthetic_token(
                                                TK_FALSE, "false", like)),
            like)});
    }

    if (tested != 0) {
        module->get_ast()->get_node(condition)->set_children(0);
        first.push_back(make_exit_unless(tested, like));
    }

    prepend(body, first);
}

// 'elif c:' runs 'c' only when everything above it was false. Written as the
// 'else' holding an 'if' it always meant, 'c' is a condition at the top of
// its own statement, where building before it is building at the right time:
//
//     if a:                    if a:
//         ...                      ...
//     elif c:          ->      else:
//         ...                      if c:
//     else:                            ...
//         ...                      else:
//                                      ...
//
// Only the first 'elif' that builds something is moved; the ones after it
// move with it, and the walk reaches them in the new 'if'
void SugarLowerer::elif_into_an_else(u32 node) {
    u32 previous = first_child(node);

    for (u32 branch = sibling_of(previous); branch != 0;
         previous = branch, branch = sibling_of(branch)) {
        if (kind_of(branch) != AST_ELIF || !hoists(first_child(branch))) {
            continue;
        }

        u32 inner = builder.make_if(token_of(branch));

        // the elif's condition and block, then every elif and else after it
        module->get_ast()->get_node(inner)->set_children(first_child(branch));

        u32 last = sibling_of(first_child(branch));

        module->get_ast()->get_node(last)->set_sibling(sibling_of(branch));

        u32 holder = builder.make_block();

        builder.add_child(holder, 0, inner);

        AstNode* rewritten = module->get_ast()->get_node(branch);

        rewritten->set_kind(AST_ELSE);
        rewritten->set_children(0);
        rewritten->set_sibling(0);

        builder.add_child(branch, 0, holder);
        return;
    }
}

void SugarLowerer::lower_template_string(u32 node, u32 block, u32 statement) {
    u32 quote = token_of(node);
    std::string name = "__ts" + std::to_string(counter++);
    u32 name_token =
        module->add_synthetic_token(TK_IDENTIFIER, name, quote);

    std::vector<u32> written;

    written.push_back(make_declaration(name_token, quote));

    for (u32 piece = first_child(node); piece != 0;
         piece = sibling_of(piece)) {
        u32 call = make_append(name_token, piece);

        if (call != 0) {
            written.push_back(call);
        }
    }

    // and the node itself becomes the use of what was just built. Rewriting
    // it in place is what keeps every other link in the tree correct: its
    // parent still points at it and it still points at its sibling
    AstNode* rewritten = module->get_ast()->get_node(node);

    rewritten->set_kind(AST_IDENTIFIER);
    rewritten->set_token(name_token);
    rewritten->set_children(0);

    insert_before(block, statement, written);
}

// 'T[]' is written form for 'Array<T>' -- record 0016 decided it and record
// 0022 named the class, and it could not be done until a prelude could put
// 'Array' in view of a file that never wrote an import (record 0033).
//
// The node is rewritten in place into the named type, so nothing downstream
// has a case for an array with no length: TypeBuilder builds an ordinary
// instantiation, record 0002 clones it, and the emitter writes a struct. What
// the emitter used to say about this shape -- 'an array with no length cannot
// be emitted yet' -- is now unreachable through the sugar.
//
// 'T[3]' is untouched. Record 0021 makes a written length a **fixed** array,
// which is not a class at all
// The one rewrite behind all three spellings: what the brackets held becomes
// the type argument list, and the node becomes the named type it always meant.
// A hash has two children and the other two have one, which is the whole
// difference between them -- so the loop is what makes one function enough.
void SugarLowerer::lower_into_generic(u32 node, const std::string& name) {
    Ast* ast = module->get_ast();
    u32 first = first_child(node);
    u32 like = token_of(first) == 0 ? token_of(node) : token_of(first);
    u32 written = module->add_synthetic_token(TK_IDENTIFIER, name, like);
    u32 open = module->add_synthetic_token(TK_LESS_THAN, "<", like);
    u32 arguments = builder.make_generic_arguments(open);
    u32 last = 0;

    // the sibling is read before the child is moved, because a child that has
    // become an argument is followed by nothing
    for (u32 child = first; child != 0; ) {
        u32 next = sibling_of(child);

        ast->get_node(child)->set_sibling(0);
        last = builder.add_child(arguments, last, child);
        child = next;
    }

    u32 identifier = builder.make_identifier(written);

    AstNode* rewritten = ast->get_node(node);

    rewritten->set_kind(AST_NAMED_TYPE);
    rewritten->set_token(written);
    rewritten->set_children(0);

    builder.add_child(node, builder.add_child(node, 0, identifier), arguments);
}

void SugarLowerer::hoist_literal(u32 node, u32 block, u32 statement) {
    u32 like = token_of(node);
    std::string name = "__ar" + std::to_string(counter++);
    u32 name_token = module->add_synthetic_token(TK_IDENTIFIER, name, like);
    u32 let_token = module->add_synthetic_token(TK_LET, "let", like);

    // the literal itself moves into the binding, and what is left where it
    // stood is a use of the name. Cloning and then rewriting in place is what
    // keeps every other link in the tree correct
    u32 moved = builder.clone(node);

    u32 binding = builder.make_binding(
        builder.make_binding_name(builder.make_identifier(name_token)), 0,
        builder.make_binding_expression(moved));

    AstNode* rewritten = module->get_ast()->get_node(node);

    rewritten->set_kind(AST_IDENTIFIER);
    rewritten->set_token(name_token);
    rewritten->set_children(0);

    insert_before(block, statement,
                  std::vector<u32>{
                      builder.make_let_declaration(let_token, binding)});
}

u32 SugarLowerer::make_declaration(u32 name_token, u32 like) {
    u32 let_token = module->add_synthetic_token(TK_LET, "let", like);
    u32 string_token =
        module->add_synthetic_token(TK_IDENTIFIER, "String", like);

    // no initialiser: record 0026 leaves a field with no value written
    // uninitialised, and String's init taking nothing is what record 0028
    // says a local of a class type needs
    u32 binding = builder.make_binding(
        builder.make_binding_name(builder.make_identifier(name_token)),
        builder.make_binding_type(
            builder.make_named_type(builder.make_identifier(string_token), 0)),
        0);

    return builder.make_let_declaration(let_token, binding);
}

u32 SugarLowerer::make_append(u32 name_token, u32 piece) {
    u32 argument = argument_of(piece);

    // '${}' whose expression did not parse. It was reported where it was
    // written and there is nothing to append
    if (argument == 0) {
        return 0;
    }

    u32 like = token_of(piece);
    u32 append_token = module->add_synthetic_token(TK_IDENTIFIER, "append",
                                                   like);
    u32 dot_token = module->add_synthetic_token(TK_DOT, ".", like);
    u32 open_token = module->add_synthetic_token(TK_LEFT_PARENTHESIS, "(",
                                                 like);

    u32 callee = builder.make_binary_operator(
        AST_DOT, dot_token, builder.make_identifier(name_token),
        builder.make_identifier(append_token));

    u32 arguments = builder.make_arguments(open_token);

    builder.add_child(arguments, 0, argument);

    return builder.make_call(open_token, callee, arguments);
}

u32 SugarLowerer::argument_of(u32 piece) {
    if (kind_of(piece) == AST_TEMPLATE_STRING_CHUNK) {
        // the chunk's token holds the text without the quotes a literal has,
        // so the literal is a token of its own rather than this one reused
        std::string text =
            std::string(module->get_token_value(token_of(piece)));

        return builder.make_literal(
            AST_STRING_LITERAL,
            module->add_synthetic_token(TK_STRING_LITERAL, quoted(text),
                                        token_of(piece)));
    }

    u32 expression = first_child(piece);

    if (expression == 0) {
        return 0;
    }

    // it leaves the interpolation and becomes an argument, so what it used to
    // be followed by is nothing
    module->get_ast()->get_node(expression)->set_sibling(0);

    return expression;
}

std::string SugarLowerer::quoted(const std::string& text) {
    std::string result = "\"";

    for (size_t i = 0; i < text.size(); i++) {
        // an escape was written for what it means and means the same inside
        // the literal, so both of its characters go through untouched -- and
        // taking the second one here is also what stops a trailing backslash
        // from escaping the quote this is about to close
        if (text[i] == '\\' && i + 1 < text.size()) {
            result += text[i];
            result += text[i + 1];
            i++;
            continue;
        }

        // legal raw text in a template written with ', and the one character
        // that would close the literal early
        if (text[i] == '"') {
            result += "\\\"";
            continue;
        }

        result += text[i];
    }

    return result + "\"";
}

void SugarLowerer::insert_before(u32 block, u32 statement,
                                 const std::vector<u32>& statements) {
    Ast* ast = module->get_ast();
    u32 previous = 0;

    for (u32 child = ast->get_node(block)->get_children();
         child != 0 && child != statement;
         child = ast->get_node(child)->get_sibling()) {
        previous = child;
    }

    for (size_t i = 0; i + 1 < statements.size(); i++) {
        ast->get_node(statements[i])->set_sibling(statements[i + 1]);
    }

    ast->get_node(statements.back())->set_sibling(statement);

    // inserting directly before the statement every time is also what orders
    // two template strings of one statement, and an inner one against the
    // outer that appends it: whatever went in first stays in front
    if (previous == 0) {
        ast->get_node(block)->set_children(statements.front());
    } else {
        ast->get_node(previous)->set_sibling(statements.front());
    }
}

// Module level, the one place with no statement to be built before: a global's
// value, a field's, a parameter's default. The four places that used to be
// refused beside it are taken apart instead (record 0061)
void SugarLowerer::refuse(u32 node, const std::string& what) {
    u32 token = token_of(node);

    module->get_logger()->error(
        module->get_tokens()->get_token(token).get_offset(),
        module->get_tokens()->get_token(token).get_length(),
        what + " is built before the statement it is written in, and this "
               "is not inside one");
}

// 'let <name> : bool = <value>'
u32 SugarLowerer::make_flag(u32 name, u32 value, u32 like) {
    u32 let_token = module->add_synthetic_token(TK_LET, "let", like);
    u32 bool_token = module->add_synthetic_token(TK_BOOL, "bool", like);

    u32 binding = builder.make_binding(
        builder.make_binding_name(builder.make_identifier(name)),
        builder.make_binding_type(builder.make_builtin_type(bool_token)),
        builder.make_binding_expression(value));

    return builder.make_let_declaration(let_token, binding);
}

// '<name> = <value>'
u32 SugarLowerer::make_assignment(u32 name, u32 value, u32 like) {
    return builder.make_binary_operator(
        AST_ASSIGNMENT, module->add_synthetic_token(TK_ASSIGNMENT, "=", like),
        builder.make_identifier(name), value);
}

// 'if not <condition>: break'
u32 SugarLowerer::make_exit_unless(u32 condition, u32 like) {
    u32 test = builder.make_unary_operator(
        AST_LOGICAL_NOT, module->add_synthetic_token(TK_NOT, "not", like),
        condition);
    u32 body = builder.make_block();

    builder.add_child(body, 0, builder.make_jump(
        AST_BREAK, module->add_synthetic_token(TK_BREAK, "break", like), 0));

    u32 branch = builder.make_if(module->add_synthetic_token(TK_IF, "if",
                                                             like));

    builder.add_child(branch, builder.add_child(branch, 0, test), body);

    return branch;
}

void SugarLowerer::prepend(u32 block, const std::vector<u32>& statements) {
    if (statements.size() == 0) {
        return;
    }

    Ast* ast = module->get_ast();

    for (size_t i = 0; i + 1 < statements.size(); i++) {
        ast->get_node(statements[i])->set_sibling(statements[i + 1]);
    }

    ast->get_node(statements.back())->set_sibling(first_child(block));
    ast->get_node(block)->set_children(statements.front());
}

AstNodeKind SugarLowerer::kind_of(u32 node) {
    return module->get_ast()->get_node(node)->get_kind();
}

u32 SugarLowerer::first_child(u32 node) {
    return module->get_ast()->get_node(node)->get_children();
}

u32 SugarLowerer::sibling_of(u32 node) {
    return module->get_ast()->get_node(node)->get_sibling();
}

u32 SugarLowerer::token_of(u32 node) {
    return module->get_ast()->get_node(node)->get_token();
}
