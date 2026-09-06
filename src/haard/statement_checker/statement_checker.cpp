#include <haard/statement_checker/statement_checker.h>

using namespace haard;

// Every compound form asks the same question as '=', because record 0018 has
// nothing that would make two different types one. One list and not fourteen
// cases, and a form added to the grammar is one line here
static bool is_assignment(AstNodeKind kind) {
    switch (kind) {
    case AST_ASSIGNMENT:
    case AST_PLUS_ASSIGNMENT:
    case AST_MINUS_ASSIGNMENT:
    case AST_TIMES_ASSIGNMENT:
    case AST_DIVISION_ASSIGNMENT:
    case AST_INTEGER_DIVISION_ASSIGNMENT:
    case AST_MODULO_ASSIGNMENT:
    case AST_BITWISE_AND_ASSIGNMENT:
    case AST_BITWISE_OR_ASSIGNMENT:
    case AST_BITWISE_XOR_ASSIGNMENT:
    case AST_BITWISE_NOT_ASSIGNMENT:
    case AST_BITWISE_LEFT_SHIFT_ASSIGNMENT:
    case AST_BITWISE_RIGHT_SHIFT_ASSIGNMENT:
    case AST_BITWISE_UNSIGNED_RIGHT_SHIFT_ASSIGNMENT:
        return true;

    default:
        break;
    }

    return false;
}

StatementChecker::StatementChecker() {
    compilation = nullptr;
    module = nullptr;
    index = 0;
}

void StatementChecker::set_compilation(Compilation* compilation) {
    this->compilation = compilation;

    typer.set_compilation(compilation);
    coercion.set_compilation(compilation);
}

void StatementChecker::check(u32 index) {
    SymbolTable* table;

    this->index = index;
    module = compilation->get_module(index);
    table = module->get_symbols();

    scope_of.clear();

    // a diagnostic may name a type before any expression has been typed --
    // 'return' with nothing after it is one -- and the typer reads the names
    // out of whichever module it last worked on
    typer.set_module(index);

    for (u32 scope = 1; scope < table->get_scope_count(); scope++) {
        u32 owner = table->get_scope(scope)->owner;

        if (owner != 0) {
            scope_of[owner] = scope;
        }
    }

    walk(module->get_ast()->get_root(), table->get_module_scope(),
         INVALID_TYPE);
}

void StatementChecker::walk(u32 node, u32 scope, u32 result) {
    if (node == 0) {
        return;
    }

    auto opened = scope_of.find(node);

    if (opened != scope_of.end()) {
        scope = opened->second;
    }

    AstNodeKind kind = kind_of(node);

    // record 0002 again: what is inside a generic that nothing instantiated
    // is not a program. Its clone is checked, with every parameter bound
    if (is_an_unbound_generic(node)) {
        return;
    }

    switch (kind) {
    case AST_FUNCTION:
        result = result_of(node);
        break;

    // a closure is not a declaration, so it has no candidate and no signature
    // to read. Nothing is known about what it gives back, and a question
    // nobody can answer is not asked
    case AST_CLOSURE:
        result = INVALID_TYPE;
        break;

    case AST_RETURN:
        check_return(node, scope, result);
        break;

    // the condition is the first child of all three: parse_conditional is one
    // function and the shape it builds is one shape
    case AST_IF:
    case AST_ELIF:
    case AST_WHILE:
        check_condition(first_child(node), scope);
        break;

    // the middle part of 'for a; b; c:'. A foreach has none, and neither does
    // 'for a; ; c:', which is a loop that runs forever and not a mistake
    case AST_FOR_CONDITION:
        check_condition(first_child(node), scope);
        break;

    // and the last part, which was typed by **nothing** until 2026-09-03 --
    // 'for i = 0; i < 3; takes_int(2.5):' passed in silence. It is written for
    // what it does and not for what it is, exactly like an expression on a
    // line of its own, and the reason it was missed is that it is not one: a
    // block holds statements and this hangs off the loop's head.
    //
    // The head needs nothing here. An assignment there declares, and a
    // declaration's initialiser belongs to the TypeCollector's second pass --
    // typing it again would report it twice
    case AST_FOR_INCREMENT:
        for (u32 child = first_child(node); child != 0;
             child = module->get_ast()->get_node(child)->get_sibling()) {
            check_expression(child, scope);
        }

        break;

    default:
        if (is_assignment(kind)) {
            check_assignment(node, scope);
        }

        break;
    }

    // the children of a block are statements, and an expression written as one
    // is typed for what it does. An assignment is one too and was checked
    // above, so it is the one shape skipped here
    if (kind == AST_BLOCK) {
        for (u32 child = first_child(node); child != 0;
             child = module->get_ast()->get_node(child)->get_sibling()) {
            check_expression(child, scope);
        }
    }

    // and then into everything, including what was just checked: the walk is
    // looking for the statements further down -- a block under an 'if', a
    // closure inside a condition, the inner assignment of 'a = b = 1'
    for (u32 child = first_child(node); child != 0;
         child = module->get_ast()->get_node(child)->get_sibling()) {
        walk(child, scope, result);
    }
}

void StatementChecker::check_return(u32 node, u32 scope, u32 result) {
    u32 expression = first_child(node);
    u32 nothing = module->get_types()->builtin(BUILTIN_VOID);

    // no signature was built, so whatever is wrong was already reported where
    // the type was written. Asking against nothing would only report it twice
    if (result == INVALID_TYPE) {
        return;
    }

    if (expression == 0) {
        if (result != nothing) {
            report(node, "expected " + typer.name_of(result) +
                   ", found nothing");
        }

        return;
    }

    // said before the expression is typed, and not after: asking a literal to
    // be void would come back as a complaint about the literal, which is not
    // what went wrong here
    if (result == nothing) {
        report(expression, "expected void, found a value");

        return;
    }

    // the return type is handed down, so 'return 1' in a 'u8' function is a
    // u8 literal and never a conversion -- record 0018's first rule. What
    // comes back INVALID_TYPE was reported by the typer on the way
    u32 given = typer.type_of(index, scope, expression, result);

    // and what comes back a real type is asked against record 0018's list and
    // not against equality: 'return d' from a function giving back a 'Base&'
    // is the upcast that record already allows, and it failed here until
    // 2026-09-03 because only a call knew the list
    // record 0031: a value given back by value is copied out of the function
    if (given != INVALID_TYPE && !coercion.may_be_copied(index, result)) {
        report(expression, typer.name_of(result)
               + " cannot be copied, and giving one back by value copies it");
        return;
    }

    if (given == INVALID_TYPE || coercion.fits(index, given, result)) {
        return;
    }

    report(expression, "expected " + typer.name_of(result) + ", found " +
           typer.name_of(given));
}

void StatementChecker::check_expression(u32 node, u32 scope) {
    // No filtering, and none is needed: the typer has no case for a statement
    // kind, so asking about an 'if' or a 'let' gives back nothing in silence
    // and descends into nothing. An assignment is the one shape that was
    // already checked above, and asking about it again is the same nothing --
    // the typer has no case for one either.
    //
    // Asked for nothing in particular, because what it gives back is thrown
    // away. The point is that everything inside it is looked at
    typer.type_of(index, scope, node, INVALID_TYPE);
}

void StatementChecker::check_condition(u32 node, u32 scope) {
    u32 wanted = module->get_types()->builtin(BUILTIN_BOOL);

    if (node == 0) {
        return;
    }

    u32 given = typer.type_of(index, scope, node, wanted);

    if (given == INVALID_TYPE || given == wanted) {
        return;
    }

    report(node, "a condition must be bool, and this is " +
           typer.name_of(given));
}

void StatementChecker::check_assignment(u32 node, u32 scope) {
    u32 target = first_child(node);
    u32 value = second_child(node);

    // the left side decides and the right side is asked to be it, which is
    // the same way a binding hands its written type down to what it was given
    u32 left = typer.type_of(index, scope, target, INVALID_TYPE);

    if (left == INVALID_TYPE) {
        return;
    }

    // Assigning to a reference writes THROUGH it, as in C++ (Hadley,
    // 2026-09-06: 'implemente T& com semântica parecida com C++'). What is
    // being assigned is the thing it names, so that is what the right side is
    // asked to be and what record 0031's question is about.
    //
    // Two things follow, and the second is a change. 'xs.at(0) = 99' works --
    // handed the 'i32&' instead, the 99 was reported as *a literal cannot be
    // i32&*, which blames the one part of that line that is fine. And
    // 'b = d' between a 'Base&' and a 'Derived&' is now **refused**: it
    // writes the Base part of a Derived and discards the rest, which is the
    // slicing record 0018 keeps off its list -- and which the binding
    // 'let sliced : Base = d' two lines above it was already refused for
    TypeTable* types = compilation->get_module(index)->get_types();

    left = types->value_of(left);

    // Record 0034, and '=' joined its table on 2026-09-06: a class may say
    // what assigning to it means. 'a = "abc"' on a String is the reason --
    // without it the char* becomes a temporary String and record 0031's
    // assignment copies that, which is two allocations for one call.
    //
    // Asked before the coercion below, so an overload beats a conversion, and
    // it answers or says nothing: a class that declares none falls through
    if (types->get_type(left)->kind == TYPE_NAMED
        && typer.overloaded(scope, node, left, value, true) != INVALID_TYPE) {
        return;
    }

    u32 right = typer.type_of(index, scope, value, left);

    // record 0031, and an assignment is the one copy that also destroys: what
    // the target held has to go before it can hold something else
    if (right != INVALID_TYPE && !coercion.may_be_copied(index, left)) {
        report(node, typer.name_of(left)
               + " cannot be copied, and an assignment copies one");
        return;
    }

    if (right == INVALID_TYPE || coercion.fits(index, right, left)) {
        return;
    }

    // Reported at the operator, the way a mismatch between two operands is,
    // because neither side is the one that is wrong. Both sides are named by
    // the **value** they are: an assignment is about what is written, and
    // 'cannot assign Derived to Base' is the slicing complaint, while
    // 'Derived& to Base&' would name a pair record 0018's list allows
    report(node, "cannot assign " + typer.name_of(types->value_of(right))
           + " to " + typer.name_of(left));
}

u32 StatementChecker::result_of(u32 node) {
    SymbolTable* table = module->get_symbols();
    u32 candidate = table->candidate_of(node);

    if (candidate == 0) {
        return INVALID_TYPE;
    }

    u32 signature = table->get_candidate(candidate)->type;

    if (signature == INVALID_TYPE) {
        return INVALID_TYPE;
    }

    Type* entry = module->get_types()->get_type(signature);

    // record 0016 puts the return last, which is what keeps it out of what
    // distinguishes two overloads and what makes it readable from here
    if (entry->kind != TYPE_FUNCTION || entry->argument_count == 0) {
        return INVALID_TYPE;
    }

    return module->get_types()->get_argument(entry->first_argument +
                                             entry->argument_count - 1);
}

bool StatementChecker::is_an_unbound_generic(u32 node) {
    Module* module = compilation->get_module(index);
    AstQuery query;

    query.set_module(module);

    SymbolTable* table = module->get_symbols();

    for (u32 parameter : query.get_generic_parameters(node)) {
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

    return false;
}

void StatementChecker::report(u32 node, const std::string& message) {
    Token& token = module->get_tokens()->get_token(
        module->get_ast()->get_node(node)->get_token());

    module->get_logger()->error(token.get_offset(), token.get_length(),
                                message);
}

AstNodeKind StatementChecker::kind_of(u32 node) {
    return (AstNodeKind) module->get_ast()->get_node(node)->get_kind();
}

u32 StatementChecker::first_child(u32 node) {
    return module->get_ast()->get_node(node)->get_children();
}

u32 StatementChecker::second_child(u32 node) {
    u32 first = first_child(node);

    return first == 0 ? 0 : module->get_ast()->get_node(first)->get_sibling();
}
