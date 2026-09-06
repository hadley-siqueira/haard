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
    walk(module->get_ast()->get_root(), 0, 0, HOIST_OK);
}

void SugarLowerer::walk(u32 node, u32 block, u32 statement,
                        Hoisting hoisting) {
    if (node == 0) {
        return;
    }

    switch (kind_of(node)) {
    // where a statement is, and so where anything lifted out of one goes.
    // Each child is both the statement and the thing being walked
    case AST_BLOCK:
        for (u32 child = first_child(node); child != 0;
             child = sibling_of(child)) {
            walk(child, node, child, HOIST_OK);
        }

        return;

    // the condition is the first child of both, and the block after it is
    // ordinary again -- statements in there have their own place to go
    case AST_WHILE:
        walk_head_apart(node, block, statement, HOIST_IN_A_LOOP_CONDITION,
                        hoisting);
        return;

    // the middle and the last part of 'for a; b; c:'. Both run every turn,
    // and neither is a block
    case AST_FOR_CONDITION:
    case AST_FOR_INCREMENT:
        walk_children(node, block, statement, HOIST_IN_A_LOOP_CONDITION);
        return;

    // The left of a short circuit is evaluated whatever happens, so it is as
    // safe as anywhere. The right is the one that may not run
    case AST_LOGICAL_AND:
        walk_head_apart(node, block, statement, hoisting, HOIST_AFTER_AND);
        return;

    case AST_LOGICAL_OR:
        walk_head_apart(node, block, statement, hoisting, HOIST_AFTER_OR);
        return;

    // 'T[]' with nothing between the brackets. 'T[3]' is a fixed array and
    // stays one (record 0021), so the length is what tells them apart
    case AST_ARRAY_TYPE:
        walk_children(node, block, statement, hoisting);

        if (sibling_of(first_child(node)) == 0) {
            lower_array_type(node);
        }

        return;

    // its interpolations first, so that a template string written inside one
    // is already a local by the time this one appends it -- and its
    // statements, inserted before the same statement, land before these
    case AST_TEMPLATE_STRING:
        walk_children(node, block, statement, hoisting);

        if (hoisting != HOIST_OK || block == 0) {
            refuse(node, hoisting);
            recover(node, block, statement);
            return;
        }

        lower_template_string(node, block, statement);
        return;

    default:
        walk_children(node, block, statement, hoisting);
        return;
    }
}

void SugarLowerer::walk_children(u32 node, u32 block, u32 statement,
                                 Hoisting hoisting) {
    u32 child = first_child(node);

    // read the sibling before walking, because lowering rewrites the node it
    // was given and a rewritten template string has no children left
    while (child != 0) {
        u32 next = sibling_of(child);

        walk(child, block, statement, hoisting);
        child = next;
    }
}

void SugarLowerer::walk_head_apart(u32 node, u32 block, u32 statement,
                                   Hoisting head, Hoisting rest) {
    u32 child = first_child(node);

    if (child == 0) {
        return;
    }

    u32 next = sibling_of(child);

    walk(child, block, statement, head);

    while (next != 0) {
        u32 after = sibling_of(next);

        walk(next, block, statement, rest);
        next = after;
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
void SugarLowerer::lower_array_type(u32 node) {
    Ast* ast = module->get_ast();
    u32 element = first_child(node);
    u32 like = token_of(element) == 0 ? token_of(node) : token_of(element);
    u32 name = module->add_synthetic_token(TK_IDENTIFIER, "Array", like);
    u32 open = module->add_synthetic_token(TK_LESS_THAN, "<", like);

    // the element leaves the array type and becomes the argument, so what it
    // used to be followed by is nothing
    ast->get_node(element)->set_sibling(0);

    u32 arguments = builder.make_generic_arguments(open);

    builder.add_child(arguments, 0, element);

    u32 identifier = builder.make_identifier(name);

    AstNode* rewritten = ast->get_node(node);

    rewritten->set_kind(AST_NAMED_TYPE);
    rewritten->set_token(name);
    rewritten->set_children(0);

    builder.add_child(node, builder.add_child(node, 0, identifier), arguments);
}

// A refused template string is still a String -- what was refused is where it
// was written, not what it is. So it becomes one, with nothing appended to it,
// and every phase after this reads an ordinary local instead of a node no
// typer has a case for.
//
// Without it the refusal is followed by a consequence of itself: an
// expression that types to nothing makes the call around it report 'no f
// takes these arguments', and one mistake reads as two. This is the parser's
// poisoned primitive one phase later, and it is safe for the same reason --
// an error was logged, so nothing is emitted from this tree.
//
// The appends are what is dropped, and dropping them is what keeps the
// recovery from inventing errors of its own: 'for i = 0; takes("${i}"); ...'
// would otherwise put a use of 'i' before the loop that declares it
void SugarLowerer::recover(u32 node, u32 block, u32 statement) {
    // written at module level, where there is no statement to be built before
    // and so nothing to recover into. It reports once as it is
    if (block == 0) {
        return;
    }

    u32 quote = token_of(node);
    std::string name = "__ts" + std::to_string(counter++);
    u32 name_token = module->add_synthetic_token(TK_IDENTIFIER, name, quote);

    AstNode* rewritten = module->get_ast()->get_node(node);

    rewritten->set_kind(AST_IDENTIFIER);
    rewritten->set_token(name_token);
    rewritten->set_children(0);

    insert_before(block, statement,
                  std::vector<u32>{make_declaration(name_token, quote)});
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

void SugarLowerer::refuse(u32 node, Hoisting hoisting) {
    // one sentence, and it says the mechanism rather than only the verdict:
    // what makes these three places different is not obvious from the source,
    // and a reader who knows a template string becomes a local built before
    // the statement can work out the rest
    std::string where;

    switch (hoisting) {
    case HOIST_IN_A_LOOP_CONDITION:
        where = "so it cannot go in a loop condition";
        break;

    case HOIST_AFTER_AND:
        where = "so it cannot go on the right of 'and'";
        break;

    case HOIST_AFTER_OR:
        where = "so it cannot go on the right of 'or'";
        break;

    case HOIST_OK:
        where = "and this is not inside one";
        break;
    }

    u32 token = token_of(node);

    module->get_logger()->error(
        module->get_tokens()->get_token(token).get_offset(),
        module->get_tokens()->get_token(token).get_length(),
        "a template string is built before the statement it is written in, " +
            where);
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
