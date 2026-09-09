#include <haard/statement_checker/statement_checker.h>
#include <haard/type_table/type_collector.h>

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

void StatementChecker::set_collector(TypeCollector* collector) {
    typer.set_collector(collector);
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

    // One declaration at a time, and only the ones this module has not been
    // through, which is what makes a second round cost nothing and report
    // nothing twice. Record 0054: a clone made while this phase runs is added
    // to the root and the phase comes round again for it
    Ast* ast = module->get_ast();
    std::set<u32>& done = checked[index];
    std::vector<u32> round;
    AstQuery query;

    query.set_module(module);

    // The list is read **before** anything is walked, and that is the whole
    // of what makes the round work. A clone is appended to the root as it is
    // made, so walking the list live would reach it in this same round --
    // before the type phase has been through its body, which is the one thing
    // the round exists to arrange. Taken as a snapshot, it waits for the next
    for (u32 declaration = ast->get_node(ast->get_root())->get_children();
         declaration != 0;
         declaration = ast->get_node(declaration)->get_sibling()) {
        if (done.count(declaration) == 0) {
            round.push_back(declaration);
            continue;
        }

        // Record 0055: the clone of a generic **method** is appended to its
        // class's body and not to the root, so a class this phase has already
        // walked is where the new declaration turns up. The class stays done
        // -- walking it again would report everything in it twice -- and the
        // member is the round's subject on its own
        for (u32 member : query.get_members(declaration)) {
            if (done.count(member) == 0) {
                round.push_back(member);
            }
        }
    }

    for (u32 node : round) {
        done.insert(node);

        // and the members with it, since walking a class walks its body: what
        // is in the tree NOW is checked by this walk, and only what is added
        // afterwards is left for a round to come
        for (u32 member : query.get_members(node)) {
            done.insert(member);
        }

        walk(node, table->get_module_scope(), INVALID_TYPE);
    }
}

// whether this module has a declaration this checker has not walked, which is
// how the round above knows to come again
bool StatementChecker::has_more(u32 index) {
    Module* holder = compilation->get_module(index);
    Ast* ast = holder->get_ast();
    std::set<u32>& done = checked[index];
    AstQuery query;

    query.set_module(holder);

    for (u32 declaration = ast->get_node(ast->get_root())->get_children();
         declaration != 0;
         declaration = ast->get_node(declaration)->get_sibling()) {
        if (done.count(declaration) == 0) {
            return true;
        }

        // record 0055's clone, which grew inside a class instead of beside it
        for (u32 member : query.get_members(declaration)) {
            if (done.count(member) == 0) {
                return true;
            }
        }
    }

    return false;
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

    case AST_SWITCH:
        check_switch(node, scope);
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

// A switch is a **pattern match**, and this is where a pattern stops being an
// expression and becomes one: the subject says what may be written, and every
// case names a variant of it.
//
// Three things are checked and each is the reason a language grows this
// statement in the first place: a pattern that names nothing is reported by
// name, a variant written twice is reported as already covered, and a switch
// that leaves a variant out with no 'default' is reported with the ones it
// left out -- which is what turns adding a variant into a list of the places
// that have to change, instead of into silence.
void StatementChecker::check_switch(u32 node, u32 scope) {
    u32 subject = first_child(node);
    u32 given = typer.type_of(index, scope, subject, INVALID_TYPE);

    if (given == INVALID_TYPE) {
        return;
    }

    TypeTable* types = module->get_types();
    Type* entry = types->get_type(types->value_of(given));

    // An integer and a char are switched over by **value**, so there are no
    // variants to name, nothing to be exhaustive about and no pattern but a
    // written number. Hadley, 2026-09-08: a String and a tuple come too, and
    // those become a chain of 'if's rather than a C++ switch
    if (entry->kind == TYPE_BUILTIN) {
        check_switch_over_a_value(node, scope, given);

        return;
    }

    if (entry->kind != TYPE_NAMED) {
        report(subject, "a switch walks an enum, and this is "
                            + typer.name_of(given));

        return;
    }

    Module* holder = compilation->get_module(entry->module);
    SymbolTable* table = holder->get_symbols();
    Candidate* found = table->get_candidate(entry->subject);
    AstQuery theirs;

    if ((SymbolKind) found->kind != SYMBOL_ENUM) {
        report(subject, "a switch walks an enum, and this is "
                            + typer.name_of(given));

        return;
    }

    theirs.set_module(holder);

    std::vector<std::string> variants;
    std::set<std::string> covered;
    std::vector<std::string> group;
    std::vector<u32> group_carries;
    AstQuery query;
    bool grouped = false;
    bool has_default = false;

    query.set_module(module);

    for (u32 member : theirs.get_members(found->ast_node)) {
        variants.push_back(theirs.get_declaration_name(member));
    }

    u32 last_case = 0;

    for (u32 child = module->get_ast()->get_node(subject)->get_sibling();
         child != 0; child = module->get_ast()->get_node(child)->get_sibling()) {
        if (kind_of(child) == AST_DEFAULT) {
            if (has_default) {
                report(child, "this switch already has a 'default'");
            }

            has_default = true;
            last_case = child;
            continue;
        }

        last_case = child;

        std::string named = check_pattern(first_child(child), scope,
                                          typer.name_of(given), variants);

        if (named.size() > 0) {
            if (covered.count(named) > 0) {
                report(first_child(child), "'" + named
                                               + "' is already covered by this "
                                                 "switch");
            }

            covered.insert(named);
        }

        std::vector<u32> carries = carried_of(entry->module, found->ast_node,
                                              named);

        check_captures(child, carries, named);

        // Hadley's rule, 2026-09-08: cases share a body by the ones above it
        // having none, and grouping cases that bind **different names** is an
        // error -- the body would read a name that half the group never bound.
        // Binding nothing at all in every one of them is the ordinary case
        // and is not that mistake
        std::vector<std::string> bound;

        for (u32 capture : query.get_captures(child)) {
            bound.push_back(std::string(module->get_token_value(
                module->get_ast()->get_node(capture)->get_token())));
        }

        if (grouped && bound != group) {
            report(child, "these cases share a block and do not bind the same "
                          "names");
        } else if (grouped && bound.size() > 0 && carries != group_carries) {
            report(child, "these cases share a block and bind the same names "
                          "for different types");
        }

        group = bound;
        group_carries = carries;
        grouped = module->get_ast()->get_node(first_child(child))->get_sibling()
                  == 0;

        if (!grouped) {
            group.clear();
            group_carries.clear();
        }
    }

    // a case with no block runs the block of the one below it, so the last one
    // has nothing to run
    if (last_case != 0 && kind_of(last_case) == AST_CASE
        && module->get_ast()->get_node(first_child(last_case))->get_sibling()
               == 0) {
        report(last_case, "this case has no block, and there is no case after "
                          "it to share one with");
    }

    if (has_default) {
        return;
    }

    std::string missing;

    for (const std::string& variant : variants) {
        if (covered.count(variant) > 0) {
            continue;
        }

        missing += (missing.size() > 0 ? ", " : "") + variant;
    }

    if (missing.size() > 0) {
        report(node, "this switch does not cover " + missing
                         + " -- every variant, or a 'default'");
    }
}

// A switch over an integer or a char: every pattern is a **written value** of
// the subject's type, which is what a C++ 'case' label has to be as well.
//
// Nothing is checked for exhaustiveness -- there is no covering every i32 --
// so a 'default' is what a program writes when it wants one, and not writing
// one is not a mistake
void StatementChecker::check_switch_over_a_value(u32 node, u32 scope,
                                                 u32 given) {
    u32 subject = first_child(node);
    BuiltinType which = (BuiltinType) module->get_types()
                            ->get_type(module->get_types()->value_of(given))
                            ->subject;
    bool has_default = false;

    if (which == BUILTIN_BOOL || which == BUILTIN_VOID
        || which == BUILTIN_F32 || which == BUILTIN_F64) {
        report(subject, "a switch walks an enum, an integer or a char, and "
                        "this is " + typer.name_of(given));

        return;
    }

    u32 last_case = 0;

    for (u32 child = module->get_ast()->get_node(subject)->get_sibling();
         child != 0; child = module->get_ast()->get_node(child)->get_sibling()) {
        last_case = child;

        if (kind_of(child) == AST_DEFAULT) {
            if (has_default) {
                report(child, "this switch already has a 'default'");
            }

            has_default = true;
            continue;
        }

        u32 pattern = first_child(child);

        // A written number and nothing else, which is C++'s rule for a label
        // and will stop being this one when a String switch arrives -- that
        // one is a chain of comparisons and takes any expression
        if (pattern != 0 && kind_of(pattern) != AST_INTEGER_LITERAL
            && kind_of(pattern) != AST_CHAR_LITERAL) {
            report(pattern, "a case over " + typer.name_of(given)
                                + " is a written value");

            continue;
        }

        typer.type_of(index, scope, pattern, given);
    }

    if (last_case != 0 && kind_of(last_case) == AST_CASE
        && module->get_ast()->get_node(first_child(last_case))->get_sibling()
               == 0) {
        report(last_case, "this case has no block, and there is no case after "
                          "it to share one with");
    }
}

// What a pattern takes apart has to be what its variant carries: a name per
// thing, or none at all. 'case Increment:' is written when the payload is not
// wanted and is not the same mistake as writing the wrong number of names
void StatementChecker::check_captures(u32 one_case,
                                      const std::vector<u32>& carries,
                                      const std::string& variant) {
    AstQuery query;

    query.set_module(module);

    std::vector<u32> captures = query.get_captures(one_case);

    if (captures.size() == 0 || variant.size() == 0) {
        return;
    }

    if (captures.size() != carries.size()) {
        report(first_child(one_case),
               "'" + variant + "' carries " + std::to_string(carries.size())
                   + (carries.size() == 1 ? " thing" : " things")
                   + ", and this takes " + std::to_string(captures.size())
                   + " apart");
    }
}

// what a variant carries, in the table of the module that declares the enum
// -- which is where two cases of one switch may be compared without
// translating anything, since they name variants of the same enum
std::vector<u32> StatementChecker::carried_of(u32 holder, u32 declaration,
                                              const std::string& variant) {
    std::vector<u32> carries;
    Module* owner = compilation->get_module(holder);
    AstQuery theirs;

    if (variant.size() == 0) {
        return carries;
    }

    theirs.set_module(owner);

    for (u32 member : theirs.get_members(declaration)) {
        if (theirs.get_declaration_name(member) != variant) {
            continue;
        }

        u32 named = owner->get_symbols()->candidate_of(member);
        u32 signature = named == 0
                            ? INVALID_TYPE
                            : owner->get_symbols()->get_candidate(named)->type;

        if (signature == INVALID_TYPE
            || owner->get_types()->get_type(signature)->kind != TYPE_FUNCTION) {
            return carries;
        }

        carries = owner->get_types()->get_arguments(signature);

        // the return type is the last one, per record 0016
        carries.pop_back();

        return carries;
    }

    return carries;
}

// The variant a pattern names, and an empty string when it names none. Two
// shapes today: a bare 'Idle' and a written 'Action.Idle', which is the same
// pair every language with this statement offers -- the short one because the
// subject already says which enum it is, and the long one for when a name in
// scope would win
std::string StatementChecker::check_pattern(u32 pattern, u32 scope,
                                            const std::string& subject,
                                            const std::vector<std::string>&
                                                variants) {
    if (pattern == 0) {
        return "";
    }

    u32 named = pattern;

    // 'case Click(x, y)' takes the variant apart, and what names it is the
    // callee -- the pattern is the shape a construction already is, read the
    // other way round
    if (kind_of(named) == AST_CALL) {
        named = first_child(named);
    }

    if (kind_of(named) == AST_DOT) {
        named = module->get_ast()->get_node(first_child(named))->get_sibling();
    }

    if (named == 0 || kind_of(named) != AST_IDENTIFIER) {
        report(pattern, "a pattern names a variant of " + subject);

        return "";
    }

    std::string text = std::string(
        module->get_token_value(module->get_ast()->get_node(named)->get_token()));

    for (const std::string& variant : variants) {
        if (variant == text) {
            return text;
        }
    }

    report(named, subject + " has no variant named '" + text + "'");

    return "";
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
    // 2026-09-06: 'implement T& too, with semantics close to C++'s'). What is
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
    // An **empty** literal is skipped here, and that is not a special case
    // about assignment: it has no type of its own, so it cannot pick an
    // overload, and the only thing that can say what it is is the written
    // type on the left. Asking first typed it with no context and reported
    // 'nothing here says what this is empty of' about 'a = []', which the
    // line below answers perfectly well
    bool empty = (kind_of(value) == AST_LIST || kind_of(value) == AST_ARRAY)
              && first_child(value) == 0;

    if (!empty && types->get_type(left)->kind == TYPE_NAMED
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
