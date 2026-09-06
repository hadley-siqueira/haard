#include <haard/parser/parser.h>

using namespace haard;

// what to call a token kind inside a message. Only the kinds the parser asks
// for need one, so this grows with the grammar
static std::string describe(TokenKind kind) {
    switch (kind) {
        case TK_IMPORT: return "'import'";
        case TK_DEF: return "'def'";
        case TK_PASS: return "'pass'";
        case TK_AT: return "'@'";
        case TK_COLON: return "':'";
        case TK_COMMA: return "','";
        case TK_IN: return "'in'";
        case TK_SEMICOLON: return "';'";
        case TK_BITWISE_OR: return "'|'";
        case TK_LEFT_CURLY_BRACKET: return "'{'";
        case TK_RIGHT_CURLY_BRACKET: return "'}'";
        case TK_GREATER_THAN: return "'>'";
        case TK_LET: return "'let'";
        case TK_CONST: return "'const'";
        case TK_AS: return "'as'";
        case TK_DOT: return "'.'";
        case TK_LEFT_PARENTHESIS: return "'('";
        case TK_TEMPLATE_STRING_END: return "the end of the template string";
        case TK_INTERPOLATION_END: return "'}'";
        case TK_RIGHT_PARENTHESIS: return "')'";
        case TK_RIGHT_SQUARE_BRACKET: return "']'";
        case TK_IDENTIFIER: return "an identifier";
        case TK_EOF: return "the end of the file";
        default: break;
    }

    return "a different token";
}

// the ast kind a literal token becomes, or AST_UNKNOWN when the token is not a
// literal at all. They all become the same shape of node — a kind and the token
// it was written as — so what separates them is only this table
// the ast kind an assignment token becomes, or AST_UNKNOWN when the token is
// not one of them. They are all the same shape of node, so what separates them
// is only this table
static AstNodeKind assignment_kind(TokenKind kind) {
    switch (kind) {
        case TK_ASSIGNMENT: return AST_ASSIGNMENT;
        case TK_PLUS_ASSIGNMENT: return AST_PLUS_ASSIGNMENT;
        case TK_MINUS_ASSIGNMENT: return AST_MINUS_ASSIGNMENT;
        case TK_TIMES_ASSIGNMENT: return AST_TIMES_ASSIGNMENT;
        case TK_DIVISION_ASSIGNMENT: return AST_DIVISION_ASSIGNMENT;
        case TK_BITWISE_NOT_ASSIGNMENT: return AST_BITWISE_NOT_ASSIGNMENT;

        case TK_INTEGER_DIVISION_ASSIGNMENT:
            return AST_INTEGER_DIVISION_ASSIGNMENT;

        case TK_MODULO_ASSIGNMENT: return AST_MODULO_ASSIGNMENT;
        case TK_BITWISE_AND_ASSIGNMENT: return AST_BITWISE_AND_ASSIGNMENT;
        case TK_BITWISE_OR_ASSIGNMENT: return AST_BITWISE_OR_ASSIGNMENT;
        case TK_BITWISE_XOR_ASSIGNMENT: return AST_BITWISE_XOR_ASSIGNMENT;

        case TK_BITWISE_LEFT_SHIFT_ASSIGNMENT:
            return AST_BITWISE_LEFT_SHIFT_ASSIGNMENT;

        case TK_BITWISE_RIGHT_SHIFT_ASSIGNMENT:
            return AST_BITWISE_RIGHT_SHIFT_ASSIGNMENT;

        case TK_BITWISE_UNSIGNED_RIGHT_SHIFT_ASSIGNMENT:
            return AST_BITWISE_UNSIGNED_RIGHT_SHIFT_ASSIGNMENT;

        default: break;
    }

    return AST_UNKNOWN;
}

// whether the token is one of the builtin types. They are words of their own so
// that 'i32' cannot quietly be a variable name, which is what made 'sizeof(i32)'
// parse by accident before they existed. Everything else a type may name — a
// 'String', a 'Node' — is still an ordinary identifier
static bool is_builtin_type(TokenKind kind) {
    switch (kind) {
        case TK_U8: case TK_U16: case TK_U32: case TK_U64:
        case TK_I8: case TK_I16: case TK_I32: case TK_I64:
        case TK_F32: case TK_F64:
        case TK_BOOL: case TK_VOID: case TK_CHAR:
            return true;

        default:
            break;
    }

    return false;
}

static AstNodeKind literal_kind(TokenKind kind) {
    switch (kind) {
        case TK_INTEGER_LITERAL: return AST_INTEGER_LITERAL;
        case TK_FLOAT_LITERAL: return AST_FLOAT_LITERAL;
        case TK_STRING_LITERAL: return AST_STRING_LITERAL;
        case TK_CHAR_LITERAL: return AST_CHAR_LITERAL;
        case TK_SYMBOL_LITERAL: return AST_SYMBOL_LITERAL;
        case TK_TRUE: return AST_TRUE;
        case TK_FALSE: return AST_FALSE;
        case TK_NULL: return AST_NULL_LITERAL;
        default: break;
    }

    return AST_UNKNOWN;
}

Parser::Parser() {
    module = nullptr;
    tokens = nullptr;
    logger = nullptr;
    current_token = 0;
    matched = 0;
    statement_first_token = 0;
    panic = false;
    open_brackets = 0;
    indentation_stack.push_back(0);
}

void Parser::set_module(Module* module) {
    this->module = module;
    this->tokens = module->get_tokens();
    this->logger = module->get_logger();

    builder.set_ast(module->get_ast());
}

u32 Parser::parse() {
    current_token = 0;
    matched = 0;
    statement_first_token = 0;
    panic = false;
    open_brackets = 0;

    indentation_stack.clear();
    indentation_stack.push_back(0);

    return parse_module();
}

//   module := declaration*
u32 Parser::parse_module() {
    u32 node = builder.make_module();
    u32 last = 0;

    while (!lookahead(TK_EOF)) {
        // the line the statement starts on, so that a failed one can take the
        // lines it opened down with it
        u32 indentation = indentation_of_current_line();
        u32 start = current_token;

        begin_statement();

        u32 child = parse_declaration();

        // one of the two places that know about panic mode: the recovery point
        if (panic) {
            synchronize(indentation, start, false);
            continue;
        }

        last = builder.add_child(node, last, child);
    }

    return node;
}

// the word a statement opens with is what picks the rule, so a token that
// opens none of them is the one error this level reports by itself
//
//   declaration := import | let_declaration | const_declaration | function
//                | class | struct | enum | union
u32 Parser::parse_declaration() {
    if (lookahead(TK_IMPORT)) {
        return parse_import();
    }

    if (lookahead(TK_LET)) {
        return parse_let_declaration();
    }

    if (lookahead(TK_CONST)) {
        return parse_const_declaration();
    }

    if (lookahead(TK_DEF)) {
        return parse_function();
    }

    if (lookahead(TK_CLASS)) {
        return parse_type_declaration(TK_CLASS, AST_CLASS);
    }

    if (lookahead(TK_STRUCT)) {
        return parse_type_declaration(TK_STRUCT, AST_STRUCT);
    }

    if (lookahead(TK_ENUM)) {
        return parse_type_declaration(TK_ENUM, AST_ENUM);
    }

    if (lookahead(TK_UNION)) {
        return parse_type_declaration(TK_UNION, AST_UNION);
    }

    error_found("a declaration", false);

    return 0;
}

//   import := 'import' import_path ('as' identifier)?
u32 Parser::parse_import() {
    u32 token = current_token;

    expect(TK_IMPORT);

    u32 path = parse_import_path();
    u32 alias = parse_import_alias();

    return builder.make_import(token, path, alias);
}

// A '*' after a dot takes everything under that path and ends it: nothing may
// follow it, which is why the loop stops there rather than going round again.
//
//   import_path := identifier ('.' identifier)* ('.' '*')?
u32 Parser::parse_import_path() {
    u32 node = builder.make_import_path();
    u32 last = builder.add_child(node, 0, parse_import_path_segment());

    // in panic the match answers false, so a broken path stops here instead of
    // reporting one error per segment
    while (match_on_same_line(TK_DOT)) {
        if (match_on_same_line(TK_TIMES)) {
            builder.add_child(node, last, builder.make_import_all(matched));
            break;
        }

        last = builder.add_child(node, last, parse_import_path_segment());
    }

    return node;
}

// a segment that is not there gives back 0, which add_child skips: an error
// leaves the path shorter, never with a hole in it
u32 Parser::parse_import_path_segment() {
    if (expect_on_same_line(TK_IDENTIFIER)) {
        return builder.make_import_path_segment(matched);
    }

    return 0;
}

u32 Parser::parse_import_alias() {
    if (!match_on_same_line(TK_AS)) {
        return 0;
    }

    if (expect_on_same_line(TK_IDENTIFIER)) {
        return builder.make_import_alias(matched);
    }

    return 0;
}

//   let_declaration := 'let' binding
u32 Parser::parse_let_declaration() {
    // read before the expect, because the calls below move 'matched'
    u32 token = current_token;

    expect(TK_LET);

    return builder.make_let_declaration(token, parse_binding());
}

//   const_declaration := 'const' binding
u32 Parser::parse_const_declaration() {
    u32 token = current_token;

    expect(TK_CONST);

    return builder.make_const_declaration(token, parse_binding());
}

// Class, struct, enum and union are one rule with four words in front of it —
// same generic parameters, same inherited type between brackets, same body of
// fields and methods. Only the word and the node kind change.
//
//   type_declaration := ('class' | 'struct' | 'enum' | 'union') identifier
//                       generic_parameters? super_type? ':' type_body
u32 Parser::parse_type_declaration(TokenKind keyword, AstNodeKind kind) {
    u32 token = current_token;
    u32 indentation = indentation_of_current_line();

    begin_statement();
    expect(keyword);

    u32 node = builder.make_type_declaration(kind, token);
    u32 last = builder.add_child(node, 0, parse_binding_name());

    last = builder.add_child(node, last, parse_generic_parameters());
    last = builder.add_child(node, last, parse_super_type());

    expect_on_same_line(TK_COLON);

    builder.add_child(node, last,
                      parse_type_body(indentation, kind == AST_ENUM
                                      ? BODY_ENUM_MEMBERS
                                      : BODY_MEMBERS));

    return node;
}

//   super_type := '(' type ')'
u32 Parser::parse_super_type() {
    if (!match_on_same_line(TK_LEFT_PARENTHESIS)) {
        return 0;
    }

    u32 token = matched;
    u32 type = parse_type();

    expect_on_same_line(TK_RIGHT_PARENTHESIS);

    return builder.make_super_type(token, type);
}

// the same shape a block has, and the same rules — 'pass' fills an empty one,
// and a body with nothing in it is an error. Only what a line may hold differs
//
//   type_body := 'pass' | member+
u32 Parser::parse_type_body(u32 header_indentation, BodyKind kind) {
    u32 node = builder.make_type_body();

    indent(header_indentation);
    parse_body(node, kind);
    dedent();

    return node;
}

// a method is a 'def' and everything else on a line of its own is a field, so
// no lookahead beyond the first token is needed
//
//   member := function | field
u32 Parser::parse_member(bool type_is_optional) {
    if (lookahead(TK_DEF)) {
        return parse_function();
    }

    return parse_field(type_is_optional);
}

// Whether the type may be left out is the one thing that differs between the
// four declarations, and Hadley settled it 2026-08-23: in a class, a struct or
// a union a field must state its type, because a field with no type gives
// nothing to lay out; in an enum it is optional, because a variant that carries
// no payload is the ordinary case.
//
// The initial value is optional in all four. The reference allowed it for a
// struct, a union and an enum but not for a class, and that unevenness was not
// kept.
//
//   field := identifier ':' type ('=' expression)?          in a class
//          | identifier (':' type)? ('=' expression)?       in an enum
u32 Parser::parse_field(bool type_is_optional) {
    u32 name = parse_binding_name();

    u32 type = type_is_optional
        ? parse_binding_type()
        : parse_param_type();

    u32 value = parse_binding_expression();

    return builder.make_field(name, type, value);
}

// The first rule with a body, and so the first one that opens a block. Two
// things to keep in mind while it grows:
//
//   - **it has exactly one exit.** The indentation stack is state, and a
//     poisoned parser returns early from everywhere, so a 'return' between the
//     indent and the dedent would leak a level onto the stack and every block
//     read after it would measure against the wrong line. The loops below stop
//     by themselves in panic, which is what makes a single exit possible;
//   - the header line's own indentation is what gets pushed, so anything
//     deeper is inside. Aligned or not — see the permissive block rule.
//
//   function := 'def' identifier generic_parameters? ':' type param* body?
u32 Parser::parse_function() {
    u32 token = current_token;
    u32 indentation = indentation_of_current_line();

    expect(TK_DEF);

    u32 node = builder.make_function(token);
    u32 last = builder.add_child(node, 0,
                                 lookahead_on_same_line(TK_OPERATOR)
                                     ? parse_operator_name()
                                     : parse_binding_name());

    last = builder.add_child(node, last, parse_generic_parameters());

    expect_on_same_line(TK_COLON);

    last = builder.add_child(node, last, parse_function_return_type());

    indent(indentation);

    // a parameter is written on a line of its own inside the function, which
    // is why the block has to be open before they are read. The old compiler
    // needed three tokens of lookahead here to tell '@name : Type' from an
    // annotation; there are no annotations yet, so the '@' is enough
    while (is_indented() && lookahead(TK_AT)) {
        last = builder.add_child(node, last, parse_param());
    }

    dedent();

    // the statements sit at the level the parameters were read at, and the
    // block pushes it again for itself. Two balanced pairs rather than one
    // that spans both, because the block is the only rule allowed to own the
    // stack and the parameters are not statements
    builder.add_child(node, last, parse_block(indentation));

    return node;
}

// 'Foo<T>' glued is a generic and 'a < b' spaced is a comparison, and this is
// the first place the language gets to say so. Nothing is ambiguous in a
// declaration header — a '<' after the name can only open this list — so the
// check is not there to disambiguate, it is there because the spacing is the
// rule and a rule nobody enforces is a suggestion
//
//   generic_parameters := '<' identifier (',' identifier)* '>'
u32 Parser::parse_generic_parameters() {
    if (!lookahead_on_same_line(TK_LESS_THAN)) {
        return 0;
    }

    if (!glued_to_previous()) {
        error_at_current("a generic parameter list is written glued to the "
                         "name it belongs to");
        return 0;
    }

    u32 token = current_token;

    expect_on_same_line(TK_LESS_THAN);

    u32 node = builder.make_generic_parameters(token);
    u32 last = builder.add_child(node, 0, parse_identifier());

    while (match_on_same_line(TK_COMMA)) {
        last = builder.add_child(node, last, parse_identifier());
    }

    expect_on_same_line(TK_GREATER_THAN);

    return node;
}

u32 Parser::parse_function_return_type() {
    u32 type = parse_type();

    if (type == 0) {
        return 0;
    }

    return builder.make_function_return_type(type);
}

// Everything deeper than the line that opened it, and the second of the two
// places that know about panic mode — the other is parse_module. Recovering
// here rather than letting the error travel outwards is what keeps a broken
// statement from taking its whole block with it.
//
// It is the only rule that pushes a level, and it has exactly one exit for the
// reason parse_function does: a return between the indent and the dedent leaks
// a level onto the stack.
//
//   block := 'pass' | statement+
u32 Parser::parse_block(u32 header_indentation) {
    u32 node = builder.make_block();

    indent(header_indentation);
    parse_body(node, BODY_INDENTED);
    dedent();

    return node;
}

// The body of a closure. It is delimited by its braces, so it pushes **no**
// indentation level and reads statements until the '}' — which is what lets a
// closure be written on one line. An 'if' inside it still opens a block of its
// own and measures against whatever level encloses the closure.
//
//   braced_block := '{' ('pass' | statement+) '}'
u32 Parser::parse_braced_block() {
    u32 node = builder.make_block();

    parse_body(node, BODY_BRACED);

    return node;
}

// The statements of a block, whichever delimits it. Both callers share this,
// because the recovery point, the 'pass' rule and the empty-block error are the
// same wherever a block is written — only what ends it differs.
//
// In panic 'is_indented' is inert, so a header whose condition failed reads no
// block at all and the error travels out to the statement that can recover from
// it, instead of being recovered inside a block that never opened. A braced
// block answers to its brace instead, and to the end of the file, so an
// unclosed one stops rather than running away.
u32 Parser::parse_body(u32 node, BodyKind kind) {
    u32 last = 0;
    bool had_lines = false;

    // A block holds statements and a statement is found by its line, so the
    // line rule is in force inside one however deep in brackets the block was
    // written: 'f( |x| { a = 1 \n + 2 } )' is two statements and not one sum.
    // Zeroing the count is begin_statement's job and every statement in here
    // does it; what this has to do is put the **enclosing** expression's count
    // back afterwards, so the brackets around the closure go on joining their
    // lines once the block has ended
    u32 enclosing_brackets = open_brackets;

    // 'pass' is the whole block, not a statement inside it: it says there is
    // nothing here, so nothing else can be
    if (inside_block(kind) && lookahead(TK_PASS)) {
        had_lines = true;
        last = builder.add_child(node, 0, parse_pass());

        // a line under it contradicts what it says. Reporting here rather than
        // letting the line fall out to the rule above is the difference
        // between naming the mistake and blaming the line for existing
        if (inside_block(kind)) {
            error_at_current("nothing can follow 'pass': it is how a block "
                             "with no statements is written");
        }
    } else {
        while (inside_block(kind)) {
            u32 indentation = indentation_of_current_line();
            u32 start = current_token;

            had_lines = true;

            begin_statement();

            u32 child = body_holds_members(kind)
                ? parse_member(kind == BODY_ENUM_MEMBERS)
                : parse_statement();

            // A statement takes the whole line, and until now only half of
            // that was enforced: everything had to be *on* the line, but
            // nothing checked that the line was finished. So 'let x = a b'
            // quietly became two statements sharing a line, and the greedier
            // the grammar got the easier that was to hit.
            //
            // The statement that did parse is kept: it is complete and correct,
            // and only what trails it is thrown away. That is more of the file
            // surviving than the usual 'the statement is dropped' recovery.
            if (!panic && leftover_on_the_line(kind)) {
                error_at_current("nothing may follow a statement on its line");
                last = builder.add_child(node, last, child);
                synchronize(indentation, start, kind == BODY_BRACED);
                continue;
            }

            if (panic) {
                synchronize(indentation, start, kind == BODY_BRACED);
                continue;
            }

            last = builder.add_child(node, last, child);
        }
    }

    // A block with nothing in it is an error: an empty body is written with
    // 'pass', which is what makes 'I meant nothing here' different from 'I
    // forgot'. The distinction below is about *messages*, not about the tree —
    // either way the block cannot be kept, because a childless block prints as
    // nothing and would read back as this very error.
    if (last == 0) {
        // there really was nothing under the header. If the lines were there
        // and failed, the error that killed them was already reported and a
        // second one would only be noise
        if (!had_lines) {
            error_found(body_holds_members(kind)
                        ? "a field, a method or 'pass'"
                        : "a statement or 'pass'", true);
        }

        poison();
    }

    open_brackets = enclosing_brackets;

    return last;
}

// whether the statement just read left something behind on its line. The brace
// that closes a braced block does not count: '|x| { x + 1 }' is one line on
// purpose, and the '}' is what ends it rather than something left over
bool Parser::leftover_on_the_line(BodyKind kind) {
    if (current().get_kind() == TK_EOF) {
        return false;
    }

    if (kind == BODY_BRACED && lookahead(TK_RIGHT_CURLY_BRACKET)) {
        return false;
    }

    return on_same_line();
}

// whether another statement of this block is still ahead
bool Parser::body_holds_members(BodyKind kind) {
    return kind == BODY_MEMBERS || kind == BODY_ENUM_MEMBERS;
}

bool Parser::inside_block(BodyKind kind) {
    if (kind != BODY_BRACED) {
        return is_indented();
    }

    return !panic
        && !lookahead(TK_RIGHT_CURLY_BRACKET)
        && current().get_kind() != TK_EOF;
}

u32 Parser::parse_pass() {
    begin_statement();
    expect(TK_PASS);

    return builder.make_pass(matched);
}

// an expression is what is left when no keyword opens the line, so it needs no
// lookahead of its own
//
//   statement := let_declaration | const_declaration
//              | if | while | for
//              | return | break | continue | yield | goto | label
//              | expression
u32 Parser::parse_statement() {
    // the same rules the module level uses: a binding is a binding wherever it
    // is written, and only what may hold one differs
    if (lookahead(TK_LET)) {
        return parse_let_declaration();
    }

    if (lookahead(TK_CONST)) {
        return parse_const_declaration();
    }

    if (lookahead(TK_IF)) {
        return parse_if();
    }

    if (lookahead(TK_WHILE)) {
        return parse_while();
    }

    if (lookahead(TK_FOR)) {
        return parse_for();
    }

    if (lookahead(TK_RETURN)) {
        return parse_jump(TK_RETURN, AST_RETURN);
    }

    if (lookahead(TK_BREAK)) {
        return parse_jump(TK_BREAK, AST_BREAK);
    }

    if (lookahead(TK_CONTINUE)) {
        return parse_jump(TK_CONTINUE, AST_CONTINUE);
    }

    if (lookahead(TK_YIELD)) {
        return parse_jump(TK_YIELD, AST_YIELD);
    }

    if (lookahead(TK_GOTO)) {
        return parse_jump(TK_GOTO, AST_GOTO);
    }

    if (lookahead(TK_LABEL)) {
        return parse_label();
    }

    return parse_expression();
}

// what a 'goto' jumps to. The name is written after the word rather than before
// a colon, so nothing here has to be told apart from an expression statement
//
//   label := 'label' identifier
u32 Parser::parse_label() {
    u32 token = current_token;

    begin_statement();
    expect(TK_LABEL);

    return builder.make_label(token, parse_identifier());
}

// The five that leave a block: one shape, and the expression after the keyword
// is optional for all of them, exactly as the old compiler had it.
//
// What decides whether there is one is the **line**: 'return' on its own is a
// return with nothing to give back, and the next line is the next statement,
// not the value. That is the same rule as everywhere else here, and it is the
// only thing standing between 'return' and swallowing whatever comes after it.
//
//   jump := ('return' | 'break' | 'continue' | 'yield' | 'goto') expression?
u32 Parser::parse_jump(TokenKind keyword, AstNodeKind kind) {
    u32 token = current_token;

    begin_statement();
    expect(keyword);

    u32 expression = 0;

    // the end of the file is not the same line as anything: without this a
    // file ending in 'return' would look for a value past its last token
    if (on_same_line() && !lookahead(TK_EOF)) {
        expression = parse_expression();
    }

    return builder.make_jump(kind, token, expression);
}

//   if := 'if' expression ':' block elif* else?
u32 Parser::parse_if() {
    u32 token = current_token;
    u32 indentation = indentation_of_current_line();

    begin_statement();
    expect(TK_IF);

    u32 node = builder.make_if(token);
    u32 last = parse_conditional(node, indentation);

    // 'is_indented' is the guard the old compiler had here and it matters: the
    // stack is back to the block that holds this 'if', so an 'elif' written
    // outside that block is not this one's. Without it a dedented 'elif' would
    // be swallowed instead of reported
    while (lookahead(TK_ELIF) && is_indented()) {
        last = builder.add_child(node, last, parse_elif());
    }

    if (lookahead(TK_ELSE) && is_indented()) {
        builder.add_child(node, last, parse_else());
    }

    return node;
}

//   elif := 'elif' expression ':' block
u32 Parser::parse_elif() {
    u32 token = current_token;
    u32 indentation = indentation_of_current_line();

    begin_statement();
    expect(TK_ELIF);

    u32 node = builder.make_elif(token);

    parse_conditional(node, indentation);

    return node;
}

//   else := 'else' ':' block
u32 Parser::parse_else() {
    u32 token = current_token;
    u32 indentation = indentation_of_current_line();

    begin_statement();
    expect(TK_ELSE);
    expect_on_same_line(TK_COLON);

    u32 node = builder.make_else(token);

    builder.add_child(node, 0, parse_block(indentation));

    return node;
}

// One word, three loops, and which one it is cannot be known until its head has
// been read — that is the reference's shape and it is kept:
//
//   for i in 0..len:          a foreach
//   for a, b in pairs:        a foreach that takes the sequence apart
//   for i = 0; i < n; i++:    the C shaped one, and the only use of ';'
//
// What separates them is whether the **last** expression of the head is an 'in'
// or a 'not in'. So the head is read into a node of its own first, and only
// then is the loop it belongs to built. The reference decided the same way but
// left the taking-apart case unfinished; here the expressions simply stay in
// the head, in order, with the 'in' last.
//
//   for := 'for' for_head (';' expression? (';' expression_list?)?)? ':' block
u32 Parser::parse_for() {
    u32 token = current_token;
    u32 indentation = indentation_of_current_line();

    begin_statement();
    expect(TK_FOR);

    u32 head = builder.make_for_head();
    u32 last_expression = 0;

    // 'for ; a < n; i++:' leaves the head empty
    if (!lookahead_on_same_line(TK_SEMICOLON)) {
        last_expression = builder.add_child(head, 0, parse_expression());

        while (match_on_same_line(TK_COMMA)) {
            last_expression = builder.add_child(head, last_expression,
                                                parse_expression());
        }
    }

    bool each = kind_of(last_expression) == AST_IN
             || kind_of(last_expression) == AST_NOT_IN;

    u32 node = each ? builder.make_for_each(token) : builder.make_for(token);
    u32 last = builder.add_child(node, 0, head);

    // a foreach has no semicolons: what would have been the condition and the
    // increment is the sequence it walks
    if (!each) {
        last = builder.add_child(node, last, parse_for_condition());
        last = builder.add_child(node, last, parse_for_increment());
    }

    expect_on_same_line(TK_COLON);

    builder.add_child(node, last, parse_block(indentation));

    return node;
}

// The node is built as soon as its ';' is read, even when nothing follows it,
// because the semicolons are what say which part is which: 'for a; ; c:' has no
// condition and still needs both of them written back.
u32 Parser::parse_for_condition() {
    if (!match_on_same_line(TK_SEMICOLON)) {
        return 0;
    }

    u32 node = builder.make_for_condition();

    if (!lookahead_on_same_line(TK_SEMICOLON)
        && !lookahead_on_same_line(TK_COLON)) {
        builder.add_child(node, 0, parse_expression());
    }

    return node;
}

u32 Parser::parse_for_increment() {
    if (!match_on_same_line(TK_SEMICOLON)) {
        return 0;
    }

    u32 node = builder.make_for_increment();

    if (lookahead_on_same_line(TK_COLON)) {
        return node;
    }

    u32 last = builder.add_child(node, 0, parse_expression());

    while (match_on_same_line(TK_COMMA)) {
        last = builder.add_child(node, last, parse_expression());
    }

    return node;
}

//   while := 'while' expression ':' block
u32 Parser::parse_while() {
    u32 token = current_token;
    u32 indentation = indentation_of_current_line();

    begin_statement();
    expect(TK_WHILE);

    u32 node = builder.make_while(token);

    parse_conditional(node, indentation);

    return node;
}

// the shape 'if', 'elif' and 'while' share: a condition, a ':' closing the
// header line, and the block under it. Gives back the last child, which is what
// an 'if' needs in order to go on appending its elif and else
u32 Parser::parse_conditional(u32 node, u32 header_indentation) {
    u32 condition = parse_expression();

    expect_on_same_line(TK_COLON);

    u32 last = builder.add_child(node, 0, condition);

    return builder.add_child(node, last, parse_block(header_indentation));
}

// The type is not optional; the value is, and it is what the caller gets when
// it leaves the argument out. The reference had neither.
//
//   param := '@' identifier ':' type ('=' expression)?
u32 Parser::parse_param() {
    u32 token = current_token;

    begin_statement();
    expect(TK_AT);

    u32 name = parse_binding_name();
    u32 type = parse_param_type();
    u32 value = parse_binding_expression();

    return builder.make_param(token, name, type, value);
}

// the type of a parameter is not optional, unlike a let binding's: a parameter
// with no type gives the caller nothing to be checked against
u32 Parser::parse_param_type() {
    if (!expect_on_same_line(TK_COLON)) {
        return 0;
    }

    u32 type = parse_type();

    if (type == 0) {
        return 0;
    }

    return builder.make_binding_type(type);
}

// the three parts are read into locals first: as arguments they would be
// evaluated in whatever order the compiler picked, and the parser reads a
// stream, so the order is the meaning
//
//   binding := binding_name binding_type? binding_expression?
u32 Parser::parse_binding() {
    u32 name = parse_binding_target();
    u32 type = parse_binding_type();
    u32 expression = parse_binding_expression();

    return builder.make_binding(name, type, expression);
}

// One name, or several between brackets when a tuple is being taken apart:
// 'let (a, b) = pair'. Without the brackets it is not a target — 'let a, b = p'
// is an error, because a comma there would have to mean two bindings sharing
// one value and it does not.
//
// This is not the binding *name* rule: a function, a class and a field all bind
// a single name and must keep doing so, which is why they call that one.
//
//   binding_target := identifier | '(' identifier (',' identifier)* ')'
u32 Parser::parse_binding_target() {
    if (!lookahead_on_same_line(TK_LEFT_PARENTHESIS)) {
        return parse_binding_name();
    }

    u32 token = current_token;

    expect_on_same_line(TK_LEFT_PARENTHESIS);

    u32 tuple = builder.make_tuple(token);
    u32 last = builder.add_child(tuple, 0, parse_identifier());

    while (match_on_same_line(TK_COMMA)) {
        last = builder.add_child(tuple, last, parse_identifier());
    }

    expect_on_same_line(TK_RIGHT_PARENTHESIS);

    return builder.make_binding_name(tuple);
}

//   binding_name := identifier
u32 Parser::parse_binding_name() {
    u32 name = parse_identifier();

    if (name == 0) {
        return 0;
    }

    return builder.make_binding_name(name);
}

// the annotation is optional, so a missing ':' is not an error. Note the space
// before it is the language's, not the printer's: 'x:i32' scans as an
// identifier followed by a symbol, and never reaches here as a type
//
//   binding_type := ':' type
u32 Parser::parse_binding_type() {
    if (!match_on_same_line(TK_COLON)) {
        return 0;
    }

    u32 type = parse_type();

    if (type == 0) {
        return 0;
    }

    return builder.make_binding_type(type);
}

//   binding_expression := '=' expression
u32 Parser::parse_binding_expression() {
    if (!match_on_same_line(TK_ASSIGNMENT)) {
        return 0;
    }

    u32 expression = parse_expression();

    if (expression == 0) {
        return 0;
    }

    return builder.make_binding_expression(expression);
}

// A template string is the one literal that is not one token: the scanner cuts
// it into the quote, the text between the interpolations, and the tokens that
// open and close each of them. So it is read as a list, and the node grows the
// way every other list node here does.
//
// The pieces do not need the line rule relaxed, even though the string may span
// lines: the scanner does not set 'newline_before' inside a template, because
// the chunk before a token is what consumed the newline. A template that spans
// five lines is still one line as far as the statement rule is concerned.
//
// 'operator' and then the operator, written against it. Record 0034.
//
// What comes back is a binding name holding an identifier, exactly as a
// written name does -- the difference is only its text, which is 'operator[]'
// and is not something an identifier can be. So no source can call one by
// name, and nothing after the parser has a case for operators.
//
//   operator_name := 'operator' operator
u32 Parser::parse_operator_name() {
    u32 word = current_token;

    expect_on_same_line(TK_OPERATOR);

    if (!joined_to_previous()) {
        error_found("the operator, written against 'operator'", true);
        return 0;
    }

    u32 at = current_token;
    AstNodeKind kind = operator_kind();

    if (kind == AST_UNKNOWN) {
        error_found("an operator that may be overloaded", true);
        return 0;
    }

    // the text is built and not read off the source, because '[]' is two
    // tokens and the space between them, if any, is not part of the name
    std::string text = operator_name(kind);

    return builder.make_binding_name(builder.make_identifier(
        module->add_synthetic_token(TK_IDENTIFIER, text, at)));
}

// consumes the operator and says which one it was, AST_UNKNOWN when the token
// is not one that may be overloaded
AstNodeKind Parser::operator_kind() {
    if (match_on_same_line(TK_LEFT_SQUARE_BRACKET)) {
        return match_on_same_line(TK_RIGHT_SQUARE_BRACKET) ? AST_INDEX
                                                           : AST_UNKNOWN;
    }

    if (match_on_same_line(TK_ASSIGNMENT)) return AST_ASSIGNMENT;
    if (match_on_same_line(TK_PLUS)) return AST_PLUS;
    if (match_on_same_line(TK_MINUS)) return AST_MINUS;
    if (match_on_same_line(TK_TIMES)) return AST_TIMES;
    if (match_on_same_line(TK_INTEGER_DIVISION)) return AST_INTEGER_DIVISION;
    if (match_on_same_line(TK_DIVISION)) return AST_DIVISION;
    if (match_on_same_line(TK_MODULO)) return AST_MODULO;
    if (match_on_same_line(TK_EQUAL)) return AST_EQUAL;
    if (match_on_same_line(TK_NOT_EQUAL)) return AST_NOT_EQUAL;
    if (match_on_same_line(TK_LESS_THAN_OR_EQUAL)) return AST_LESS_THAN_OR_EQUAL;
    if (match_on_same_line(TK_LESS_THAN)) return AST_LESS_THAN;

    if (match_on_same_line(TK_GREATER_THAN_OR_EQUAL)) {
        return AST_GREATER_THAN_OR_EQUAL;
    }

    if (match_on_same_line(TK_GREATER_THAN)) return AST_GREATER_THAN;

    return AST_UNKNOWN;
}

bool Parser::joined_to_previous() {
    Token& before = tokens->get_token(current_token - 1);

    return before.get_offset() + before.get_length()
           == tokens->get_token(current_token).get_offset();
}

//   template_string := BEGIN (chunk | interpolation)* END
u32 Parser::parse_template_string() {
    u32 token = current_token;

    expect_on_same_line(TK_TEMPLATE_STRING_BEGIN);

    u32 node = builder.make_template_string(token);
    u32 last = 0;

    // the condition and the two rules test the same thing, so a round that
    // enters the loop always consumes a token. Without that a scanner left in a
    // strange state after an unterminated string would spin here, which is what
    // the runner's timeout exists to catch
    while (lookahead_on_same_line(TK_TEMPLATE_STRING_CHUNK)
           || lookahead_on_same_line(TK_INTERPOLATION_BEGIN)) {
        if (lookahead_on_same_line(TK_TEMPLATE_STRING_CHUNK)) {
            last = builder.add_child(node, last, parse_template_string_chunk());
        } else {
            last = builder.add_child(node, last, parse_interpolation());
        }
    }

    expect_on_same_line(TK_TEMPLATE_STRING_END);

    return node;
}

// the scanner puts a chunk on both sides of every interpolation, so '${x}' is
// really an empty chunk, the interpolation, and another empty chunk. An empty
// one is text that was never written, and a node for it would be noise in every
// tree: the token is consumed and 0 given back, which add_child skips
u32 Parser::parse_template_string_chunk() {
    if (!match_on_same_line(TK_TEMPLATE_STRING_CHUNK)) {
        return 0;
    }

    if (tokens->get_token(matched).get_length() == 0) {
        return 0;
    }

    return builder.make_template_string_chunk(matched);
}

//   interpolation := '${' expression '}'
u32 Parser::parse_interpolation() {
    u32 token = current_token;

    expect_on_same_line(TK_INTERPOLATION_BEGIN);

    u32 expression = parse_expression();

    expect_on_same_line(TK_INTERPOLATION_END);

    return builder.make_interpolation(token, expression);
}

// A grammar of its own, and it only meets the expression grammar in one place:
// the size of an array, 'T[n]'. Nothing here is ambiguous with an expression,
// because a type is only ever read where a type is the only thing allowed —
// after ':', after '->', after 'as'.
//
//   type := function_type
u32 Parser::parse_type() {
    return parse_function_type();
}

// '(i32, i32) -> f64' is a function taking two i32 and giving back an f64: the
// parameters arrive as a tuple and the arrow separates them from the return.
// The last child is the return type and everything before it is a parameter,
// so the chain 'A -> B -> C' also parses, with A and B as parameters.
//
//   function_type := tuple_type ('->' tuple_type)*
u32 Parser::parse_function_type() {
    u32 type = parse_tuple_type();

    if (!lookahead_on_same_line(TK_ARROW)) {
        return type;
    }

    u32 token = current_token;
    u32 node = builder.make_function_type(token);
    u32 last = builder.add_child(node, 0, type);

    while (match_on_same_line(TK_ARROW)) {
        last = builder.add_child(node, last, parse_tuple_type());
    }

    return node;
}

// '(T)' is a tuple of one element: unlike an expression, where '(a)' groups and
// only '(a,)' is a tuple, the brackets in a type always build one. Note the
// postfix below does not reach a tuple, which is where the reference left it:
// '(A, B)*' does not parse.
//
//   tuple_type := '(' type (',' type)* ')' | primary_type
u32 Parser::parse_tuple_type() {
    if (!lookahead_on_same_line(TK_LEFT_PARENTHESIS)) {
        return parse_primary_type();
    }

    u32 token = current_token;

    expect_on_same_line(TK_LEFT_PARENTHESIS);

    u32 node = builder.make_tuple_type(token);
    u32 last = builder.add_child(node, 0, parse_type());

    while (match_on_same_line(TK_COMMA)) {
        last = builder.add_child(node, last, parse_type());
    }

    expect_on_same_line(TK_RIGHT_PARENTHESIS);

    return node;
}

//   primary_type := (named_type | '[' type ']' | '{' type ':' type '}') postfix*
u32 Parser::parse_primary_type() {
    u32 type = 0;

    if (lookahead_on_same_line(TK_LEFT_SQUARE_BRACKET)) {
        u32 token = current_token;

        expect_on_same_line(TK_LEFT_SQUARE_BRACKET);

        u32 element = parse_type();

        expect_on_same_line(TK_RIGHT_SQUARE_BRACKET);
        type = builder.make_list_type(token, element);
    } else if (lookahead_on_same_line(TK_LEFT_CURLY_BRACKET)) {
        u32 token = current_token;

        expect_on_same_line(TK_LEFT_CURLY_BRACKET);

        u32 key = parse_type();

        expect_on_same_line(TK_COLON);

        u32 value = parse_type();

        expect_on_same_line(TK_RIGHT_CURLY_BRACKET);
        type = builder.make_hash_type(token, key, value);
    } else if (is_builtin_type(current().get_kind())
               && lookahead_on_same_line(current().get_kind())) {
        u32 token = current_token;

        advance();
        type = builder.make_builtin_type(token);
    } else {
        type = parse_named_type();
    }

    return parse_type_postfix(type);
}

// Left associative, each round wrapping what came before it, so 'i32*[]' is an
// array of pointers. '**' is one token and makes two pointers, the same shape
// the unary '**' has in an expression.
//
//   postfix := '*' | '**' | '&' | '[' expression? ']'
u32 Parser::parse_type_postfix(u32 type) {
    while (true) {
        if (match_on_same_line(TK_TIMES)) {
            type = builder.make_pointer_type(matched, type);
        } else if (match_on_same_line(TK_POWER)) {
            u32 oper = matched;

            type = builder.make_pointer_type(oper, type);
            type = builder.make_pointer_type(oper, type);
        } else if (match_on_same_line(TK_BITWISE_AND)) {
            type = builder.make_reference_type(matched, type);
        } else if (lookahead_on_same_line(TK_LEFT_SQUARE_BRACKET)) {
            u32 token = current_token;
            u32 size = 0;

            expect_on_same_line(TK_LEFT_SQUARE_BRACKET);

            // 'T[]' is an array of no stated length; 'T[n]' states it, and
            // that 'n' is the one place a type reads an expression
            if (!lookahead_on_same_line(TK_RIGHT_SQUARE_BRACKET)) {
                size = parse_expression();
            }

            expect_on_same_line(TK_RIGHT_SQUARE_BRACKET);
            type = builder.make_array_type(token, type, size);
        } else {
            break;
        }
    }

    return type;
}

// the name may be scoped, which is the same shape an expression writes, so the
// scope rule is reused rather than repeated
//
//   named_type := scope generic_arguments?
u32 Parser::parse_named_type() {
    u32 name = parse_scope();

    if (name == 0) {
        return 0;
    }

    return builder.make_named_type(name, parse_generic_arguments());
}

// the one message both ends of the rule report
static const char* generic_spacing_message =
    "a generic argument list is written glued, with no space at its brackets";

// 'Foo<T>' glued is a generic — the rule from the declaration header, applied
// to the other end. Spaced, it is nothing a type can mean, so it is named
// rather than left to fail somewhere further along.
//
//   generic_arguments := '<' type (',' type)* '>'
u32 Parser::parse_generic_arguments() {
    if (!lookahead_on_same_line(TK_LESS_THAN)) {
        return 0;
    }

    if (!glued_to_previous()) {
        error_at_current(generic_spacing_message);
        return 0;
    }

    u32 token = current_token;

    expect_on_same_line(TK_LESS_THAN);

    // and the list against the '<'. 'Array< i32 >' is not a generic — the rule
    // is about the brackets, not only about where the '<' sits.
    //
    // The closing side is not checked, and that is deliberate: the '>' is where
    // nested lists are carved apart, so after 'A<B<i32>>' has been split the
    // token before the outer '>' is no longer the one the source wrote. A glue
    // test there would have to know about the carving. So 'Array<i32 >' is
    // accepted today, the same kind of leniency as 'a+b'
    if (!glued_to_previous()) {
        error_at_current(generic_spacing_message);
        return 0;
    }

    u32 node = builder.make_generic_arguments(token);
    u32 last = builder.add_child(node, 0, parse_type());

    while (match_on_same_line(TK_COMMA)) {
        last = builder.add_child(node, last, parse_type());
    }

    expect_generic_close();

    return node;
}

// The angles that close nested generics arrive glued: 'A<B<i32>>' ends in one
// '>>' and 'A<B<C<i32>>>' in one '>>>', because the scanner takes the longest
// operator it can and has no idea it is inside a type.
//
// So there is no '>' to match, and one is carved off the front of the token
// **in place**: its offset moves forward and its length shrinks, leaving a real
// '>>' or '>' sitting exactly where the source has it. Every diagnostic after
// this still points at the right column, which a counter of pending angles
// would not manage.
bool Parser::expect_generic_close() {
    if (panic) {
        return false;
    }

    if (match_on_same_line(TK_GREATER_THAN)) {
        return true;
    }

    Token& token = current();
    TokenKind kind = token.get_kind();

    bool splittable = on_same_line()
        && (kind == TK_BITWISE_RIGHT_SHIFT
            || kind == TK_BITWISE_UNSIGNED_RIGHT_SHIFT);

    if (!splittable) {
        error_expected(TK_GREATER_THAN, true);
        return false;
    }

    token.set_offset(token.get_offset() + 1);
    token.set_length(token.get_length() - 1);
    token.set_kind(kind == TK_BITWISE_RIGHT_SHIFT
                   ? TK_GREATER_THAN
                   : TK_BITWISE_RIGHT_SHIFT);

    return true;
}

u32 Parser::parse_expression() {
    return parse_assignment_expression();
}

// **Right associative**, so 'a = b = c' is 'a = (b = c)'. The old compiler wrote
// this level as a left folding loop, which made it '(a = b) = c' — Hadley
// confirmed 2026-08-23 that the reference is wrong here, and this is the
// correction. It is the one level of the cascade that recurses on itself
// instead of looping, and that is what right associativity looks like.
//
//   assignment := cast (assignment_operator assignment)?
u32 Parser::parse_assignment_expression() {
    u32 node = parse_cast_expression();

    TokenKind token_kind = current().get_kind();
    AstNodeKind kind = assignment_kind(token_kind);

    if (kind == AST_UNKNOWN || !match_on_same_line(token_kind)) {
        return node;
    }

    u32 oper = matched;
    u32 right = parse_assignment_expression();

    return builder.make_binary_operator(kind, oper, node, right);
}

// the type is whatever parse_type reads, which is one identifier for now: a
// cast to 'List<T>' waits on the type grammar
//
//   cast := logical_or ('as' type)?
u32 Parser::parse_cast_expression() {
    u32 node = parse_logical_or_expression();

    if (!match_on_same_line(TK_AS)) {
        return node;
    }

    u32 oper = matched;
    u32 type = parse_type();

    return builder.make_binary_operator(AST_CAST, oper, node, type);
}

// 'or' and '||' are the same operator with two spellings, so they build the
// same node kind. Which one was written stays in the token, and that is what
// the printer reads back — the same arrangement as a template string's quote
//
//   logical_or := logical_and (('or' | '||') logical_and)*
u32 Parser::parse_logical_or_expression() {
    u32 node = parse_logical_and_expression();

    while (match_on_same_line(TK_OR) || match_on_same_line(TK_LOGICAL_OR)) {
        u32 oper = matched;
        u32 right = parse_logical_and_expression();

        node = builder.make_binary_operator(AST_LOGICAL_OR, oper, node, right);
    }

    return node;
}

//   logical_and := equality (('and' | '&&') equality)*
u32 Parser::parse_logical_and_expression() {
    u32 node = parse_equality_expression();

    while (match_on_same_line(TK_AND) || match_on_same_line(TK_LOGICAL_AND)) {
        u32 oper = matched;
        u32 right = parse_equality_expression();

        node = builder.make_binary_operator(AST_LOGICAL_AND, oper, node, right);
    }

    return node;
}

//   equality := relational (('==' | '!=') relational)*
u32 Parser::parse_equality_expression() {
    u32 node = parse_relational_expression();

    while (true) {
        AstNodeKind kind;

        if (match_on_same_line(TK_EQUAL)) {
            kind = AST_EQUAL;
        } else if (match_on_same_line(TK_NOT_EQUAL)) {
            kind = AST_NOT_EQUAL;
        } else {
            break;
        }

        u32 oper = matched;
        u32 right = parse_relational_expression();

        node = builder.make_binary_operator(kind, oper, node, right);
    }

    return node;
}

// 'not in' is two tokens on purpose and is combined here, which is the decision
// recorded for the scanner: it emits no TK_NOT_IN.
//
//   relational := range (('<' | '>' | '<=' | '>=' | 'in' | 'not' 'in') range)*
u32 Parser::parse_relational_expression() {
    u32 node = parse_range_expression();

    while (true) {
        // Spacing decides generic from comparison, and this is where what is
        // left of it lands. A '<' glued to a **name** never reaches here —
        // the primary rule has already taken it as a type argument list. So
        // anything glued that gets this far is glued to something that cannot
        // carry one: a literal, a closing bracket, a call's result.
        if ((lookahead_on_same_line(TK_LESS_THAN)
             || lookahead_on_same_line(TK_GREATER_THAN))
            && glued_to_previous()) {
            error_at_current("a comparison is written with spaces around it; "
                             "glued, this opens a generic argument list, and "
                             "what is in front of it cannot take one");
            break;
        }

        AstNodeKind kind;

        if (match_on_same_line(TK_LESS_THAN)) {
            kind = AST_LESS_THAN;
        } else if (match_on_same_line(TK_GREATER_THAN)) {
            kind = AST_GREATER_THAN;
        } else if (match_on_same_line(TK_LESS_THAN_OR_EQUAL)) {
            kind = AST_LESS_THAN_OR_EQUAL;
        } else if (match_on_same_line(TK_GREATER_THAN_OR_EQUAL)) {
            kind = AST_GREATER_THAN_OR_EQUAL;
        } else if (match_on_same_line(TK_IN)) {
            kind = AST_IN;
        } else if (match_on_same_line(TK_NOT)) {
            kind = AST_NOT_IN;

            u32 oper = matched;

            expect_on_same_line(TK_IN);

            u32 right = parse_range_expression();

            node = builder.make_binary_operator(kind, oper, node, right);
            continue;
        } else {
            break;
        }

        u32 oper = matched;
        u32 right = parse_range_expression();

        node = builder.make_binary_operator(kind, oper, node, right);
    }

    return node;
}

// Ruby's meaning, not Rust's: '..' includes the end and '...' excludes it
//
//   range := arith (('..' | '...') arith)*
u32 Parser::parse_range_expression() {
    u32 node = parse_arith_expression();

    while (true) {
        AstNodeKind kind;

        if (match_on_same_line(TK_INCLUSIVE_RANGE)) {
            kind = AST_INCLUSIVE_RANGE;
        } else if (match_on_same_line(TK_EXCLUSIVE_RANGE)) {
            kind = AST_EXCLUSIVE_RANGE;
        } else {
            break;
        }

        u32 oper = matched;
        u32 right = parse_arith_expression();

        node = builder.make_binary_operator(kind, oper, node, right);
    }

    return node;
}

// left associative, and the loosest level written so far, so this is where the
// cascade starts
//
//   arith_expression := term_expression (('+' | '-') term_expression)*
u32 Parser::parse_arith_expression() {
    u32 node = parse_term_expression();

    // in panic the matches answer false, so a broken operand ends the chain
    // here instead of reporting one error per operator
    while (true) {
        AstNodeKind kind;

        if (match_on_same_line(TK_PLUS)) {
            kind = AST_PLUS;
        } else if (match_on_same_line(TK_MINUS)) {
            kind = AST_MINUS;
        } else {
            break;
        }

        u32 oper = matched;
        u32 right = parse_term_expression();

        node = builder.make_binary_operator(kind, oper, node, right);
    }

    return node;
}

// a term is the level that binds tighter than '+' and '-', which is what makes
// 'a + b * c' fold the product first. Left associative like the level above it,
// so 'a / b / c' is '(a / b) / c'
//
//   term_expression := power_expression (('*' | '/' | '//' | '%') power_expression)*
u32 Parser::parse_term_expression() {
    u32 node = parse_power_expression();

    while (true) {
        AstNodeKind kind;

        if (match_on_same_line(TK_TIMES)) {
            kind = AST_TIMES;
        } else if (match_on_same_line(TK_DIVISION)) {
            kind = AST_DIVISION;
        } else if (match_on_same_line(TK_INTEGER_DIVISION)) {
            kind = AST_INTEGER_DIVISION;
        } else if (match_on_same_line(TK_MODULO)) {
            kind = AST_MODULO;
        } else {
            break;
        }

        u32 oper = matched;
        u32 right = parse_power_expression();

        node = builder.make_binary_operator(kind, oper, node, right);
    }

    return node;
}

// Everything from here down to the unary operators is the old compiler's
// precedence, and it is **deliberately not C's**: '**', '|', '^', '&' and the
// shifts all bind tighter than '*' and '/', so 'a + b & c' is 'a + (b & c)' and
// '2 * 3 ** 4' is '2 * (3 ** 4)'. In C the bitwise operators are looser than
// the arithmetic ones and this order would be wrong. It is not an accident and
// it is not to be corrected.
//
// '**' is left associative here, the way the old compiler wrote it, even though
// mathematics reads a power tower from the right.
//
//   power_expression := bitwise_or_expression ('**' bitwise_or_expression)*
u32 Parser::parse_power_expression() {
    u32 node = parse_bitwise_or_expression();

    while (match_on_same_line(TK_POWER)) {
        u32 oper = matched;
        u32 right = parse_bitwise_or_expression();

        node = builder.make_binary_operator(AST_POWER, oper, node, right);
    }

    return node;
}

//   bitwise_or_expression := bitwise_xor_expression ('|' bitwise_xor_expression)*
u32 Parser::parse_bitwise_or_expression() {
    u32 node = parse_bitwise_xor_expression();

    while (match_on_same_line(TK_BITWISE_OR)) {
        u32 oper = matched;
        u32 right = parse_bitwise_xor_expression();

        node = builder.make_binary_operator(AST_BITWISE_OR, oper, node, right);
    }

    return node;
}

//   bitwise_xor_expression := bitwise_and_expression ('^' bitwise_and_expression)*
u32 Parser::parse_bitwise_xor_expression() {
    u32 node = parse_bitwise_and_expression();

    while (match_on_same_line(TK_BITWISE_XOR)) {
        u32 oper = matched;
        u32 right = parse_bitwise_and_expression();

        node = builder.make_binary_operator(AST_BITWISE_XOR, oper, node, right);
    }

    return node;
}

// the '&' that stands between two operands. The one that opens an operand is
// the address-of below, and what separates them is only where they are read
//
//   bitwise_and_expression := shift_expression ('&' shift_expression)*
u32 Parser::parse_bitwise_and_expression() {
    u32 node = parse_shift_expression();

    while (match_on_same_line(TK_BITWISE_AND)) {
        u32 oper = matched;
        u32 right = parse_shift_expression();

        node = builder.make_binary_operator(AST_BITWISE_AND, oper, node, right);
    }

    return node;
}

// '>>' keeps the sign and '>>>' fills with zeroes, the way Java writes them
//
//   shift_expression := unary_expression (('<<' | '>>' | '>>>') unary_expression)*
u32 Parser::parse_shift_expression() {
    u32 node = parse_unary_expression();

    while (true) {
        AstNodeKind kind;

        if (match_on_same_line(TK_BITWISE_LEFT_SHIFT)) {
            kind = AST_BITWISE_LEFT_SHIFT;
        } else if (match_on_same_line(TK_BITWISE_RIGHT_SHIFT)) {
            kind = AST_BITWISE_RIGHT_SHIFT;
        } else if (match_on_same_line(TK_BITWISE_UNSIGNED_RIGHT_SHIFT)) {
            kind = AST_BITWISE_UNSIGNED_RIGHT_SHIFT;
        } else {
            break;
        }

        u32 oper = matched;
        u32 right = parse_unary_expression();

        node = builder.make_binary_operator(kind, oper, node, right);
    }

    return node;
}

// Right associative by recursion: '- -a' negates a negation and '!!x' is two
// nots. There is no loop here because a prefix operator applies to whatever
// comes after it, however many of them there are.
//
// 'not' and '!' are two node kinds, as they are two token kinds — the language
// keeps them apart and this is not the place to merge them.
//
// '**p' is the old compiler's shape: two dereferences, both carrying the same
// '**' token, rather than a node kind of its own.
//
//   unary_expression := ('!' | 'not' | '&' | '*' | '**' | '~' | '-' | '+'
//                       | '++' | '--') unary_expression
//                     | postfix_expression
u32 Parser::parse_unary_expression() {
    AstNodeKind kind;

    // the three that are words rather than symbols. They sit at this level in
    // the reference too, and each takes an operand the same way the symbols do
    if (lookahead_on_same_line(TK_NEW)) {
        return parse_new();
    }

    if (lookahead_on_same_line(TK_DELETE)) {
        return parse_delete();
    }

    if (lookahead_on_same_line(TK_SIZEOF)) {
        return parse_sizeof();
    }

    if (match_on_same_line(TK_LOGICAL_NOT)) {
        kind = AST_LOGICAL_NOT_OPERATOR;
    } else if (match_on_same_line(TK_NOT)) {
        kind = AST_LOGICAL_NOT;
    } else if (match_on_same_line(TK_BITWISE_AND)) {
        kind = AST_ADDRESS_OF;
    } else if (match_on_same_line(TK_TIMES)) {
        kind = AST_DEREFERENCE;
    } else if (match_on_same_line(TK_POWER)) {
        // one token, two dereferences
        u32 oper = matched;
        u32 operand = parse_unary_expression();

        operand = builder.make_unary_operator(AST_DEREFERENCE, oper, operand);

        return builder.make_unary_operator(AST_DEREFERENCE, oper, operand);
    } else if (match_on_same_line(TK_BITWISE_NOT)) {
        kind = AST_BITWISE_NOT;
    } else if (match_on_same_line(TK_MINUS)) {
        kind = AST_UNARY_MINUS;
    } else if (match_on_same_line(TK_PLUS)) {
        kind = AST_UNARY_PLUS;
    } else if (match_on_same_line(TK_INCREMENT)) {
        kind = AST_PRE_INCREMENT;
    } else if (match_on_same_line(TK_DECREMENT)) {
        kind = AST_PRE_DECREMENT;
    } else {
        return parse_postfix_expression();
    }

    u32 oper = matched;
    u32 operand = parse_unary_expression();

    return builder.make_unary_operator(kind, oper, operand);
}

// 'new T' allocates one; 'new T(a, b)' hands the arguments to it. The type is
// the type grammar's, so 'new Node<i32>*' is written the way any other type is
//
//   new := 'new' type arguments?
u32 Parser::parse_new() {
    u32 token = current_token;

    expect_on_same_line(TK_NEW);

    u32 type = parse_type();
    u32 arguments = 0;

    if (lookahead_on_same_line(TK_LEFT_PARENTHESIS)) {
        arguments = parse_arguments();
    }

    return builder.make_new(token, type, arguments);
}

// 'delete x' and 'delete[] x', the two C++ writes. The brackets are empty and
// are the whole difference between them
//
//   delete := 'delete' '[' ']'? expression
u32 Parser::parse_delete() {
    u32 token = current_token;

    expect_on_same_line(TK_DELETE);

    AstNodeKind kind = AST_DELETE;

    if (match_on_same_line(TK_LEFT_SQUARE_BRACKET)) {
        expect_on_same_line(TK_RIGHT_SQUARE_BRACKET);
        kind = AST_DELETE_ARRAY;
    }

    return builder.make_unary_operator(kind, token, parse_unary_expression());
}

// It takes a **type**, which is what asking for a size means. The reference took
// an expression, and that only ever worked because 'i32' was an ordinary name
// there; now that the sized types are words of their own it could not, and a
// type is the honest answer anyway — 'sizeof(i32*)' and 'sizeof(Array<i32>)'
// both work, where the reference managed neither.
//
// A plain name is still a type as far as the grammar is concerned, so
// 'sizeof(x)' parses; 'sizeof(a.b)' does not.
//
//   sizeof := 'sizeof' '(' type ')'
u32 Parser::parse_sizeof() {
    u32 token = current_token;

    expect_on_same_line(TK_SIZEOF);
    expect_on_same_line(TK_LEFT_PARENTHESIS);

    u32 type = parse_type();

    expect_on_same_line(TK_RIGHT_PARENTHESIS);

    return builder.make_sizeof(token, type);
}

// Tighter than every binary operator and left associative, so 'a.b.c' is
// '(a.b).c' and 'f(x)[0]' indexes what the call gave back. Each round wraps
// what came before it, which is what makes the loop enough.
//
// The old compiler asked for the same line only on '(', '++' and '--', leaving
// '.', '->' and '[' free to continue on the next line — a method chain written
// one call per line. Here every one of them wants the same line, because that
// is what the rest of this parser does and a continuation line is still an
// open question; see the note on binary operators.
//
//   postfix_expression := primary_expression postfix*
//   postfix := '.' identifier | '->' identifier | '[' expression ']'
//            | arguments | '++' | '--'
u32 Parser::parse_postfix_expression() {
    u32 node = parse_primary_expression();

    // in panic every match below answers false, so a broken operand ends the
    // chain here instead of reporting one error per operator
    while (true) {
        // the operator's token goes into a local before the member is read:
        // as arguments the two would be evaluated in whatever order the
        // compiler picked, and reading the member moves 'matched'
        if (match_on_same_line(TK_DOT)) {
            u32 oper = matched;
            u32 member = parse_identifier();

            node = builder.make_binary_operator(AST_DOT, oper, node, member);
        } else if (match_on_same_line(TK_ARROW)) {
            u32 oper = matched;
            u32 member = parse_identifier();

            node = builder.make_binary_operator(AST_ARROW, oper, node, member);
        } else if (lookahead_on_same_line(TK_LEFT_SQUARE_BRACKET)) {
            u32 token = current_token;

            expect_on_same_line(TK_LEFT_SQUARE_BRACKET);
            open_brackets++;

            u32 subscript = parse_expression();

            expect_on_same_line(TK_RIGHT_SQUARE_BRACKET);
            open_brackets--;
            node = builder.make_index(token, node, subscript);
        } else if (lookahead_on_same_line(TK_LEFT_PARENTHESIS)) {
            u32 token = current_token;

            node = builder.make_call(token, node, parse_arguments());
        } else if (match_on_same_line(TK_INCREMENT)) {
            node = builder.make_unary_operator(AST_POST_INCREMENT, matched,
                                               node);
        } else if (match_on_same_line(TK_DECREMENT)) {
            node = builder.make_unary_operator(AST_POST_DECREMENT, matched,
                                               node);
        } else {
            break;
        }
    }

    return node;
}

//   arguments := '(' (expression (',' expression)*)? ')'
u32 Parser::parse_arguments() {
    u32 token = current_token;

    expect_on_same_line(TK_LEFT_PARENTHESIS);
    open_brackets++;

    u32 node = builder.make_arguments(token);

    // 'f()' is a call with no arguments, not a call with one that is missing
    if (!lookahead_on_same_line(TK_RIGHT_PARENTHESIS)) {
        u32 last = builder.add_child(node, 0, parse_expression());

        while (match_on_same_line(TK_COMMA)) {
            last = builder.add_child(node, last, parse_expression());
        }
    }

    expect_on_same_line(TK_RIGHT_PARENTHESIS);
    open_brackets--;

    return node;
}

//   primary_expression := parenthesis_or_tuple | list | array_or_hash
//                       | closure | 'this' | template_string | literal | scope
u32 Parser::parse_primary_expression() {
    if (lookahead_on_same_line(TK_LEFT_PARENTHESIS)) {
        return parse_parenthesis_or_tuple();
    }

    if (lookahead_on_same_line(TK_LEFT_SQUARE_BRACKET)) {
        return parse_list();
    }

    if (lookahead_on_same_line(TK_LEFT_CURLY_BRACKET)) {
        return parse_array_or_hash();
    }

    // The '|' that opens a closure. The one that stands between two operands is
    // the bitwise or, and what separates them is only where they are read — so
    // a '||' met here cannot be a logical or either: it is an empty parameter
    // list, arriving glued because the scanner has no reason to split it.
    if (lookahead_on_same_line(TK_BITWISE_OR)
        || lookahead_on_same_line(TK_LOGICAL_OR)) {
        return parse_closure();
    }

    if (match_on_same_line(TK_THIS)) {
        return builder.make_this(matched);
    }

    if (lookahead_on_same_line(TK_TEMPLATE_STRING_BEGIN)) {
        return parse_template_string();
    }

    // a token that is not a literal is left where it is, so the scope rule
    // gets to see it. In panic this answers 0 as well, and the branches below
    // are inert too, so every path gives the same nothing
    u32 literal = parse_literal();

    if (literal != 0) {
        return literal;
    }

    if (lookahead_on_same_line(TK_SCOPE)
        || lookahead_on_same_line(TK_IDENTIFIER)) {
        u32 name = parse_scope();

        // 'make<i32>()' — a generic called with its types written out. The
        // list is only read when the '<' is glued to the name, which is the
        // spacing rule doing the disambiguating: spaced, it is a comparison
        // and belongs to the relational level instead.
        //
        // Only a name takes one. A literal cannot, which is what keeps the
        // relational level's own check reachable for '1 < 2' written glued.
        if (name != 0
            && lookahead_on_same_line(TK_LESS_THAN)
            && glued_to_previous()) {
            return builder.make_generic_name(name, parse_generic_arguments());
        }

        return name;
    }

    // reported here rather than left to the scope rule, which would say it
    // expected an identifier: an identifier is one of the five things allowed
    // in this position, not the only one
    error_found("an expression", true);

    return 0;
}

// The parentheses are kept in the tree as a node of their own, rather than
// dissolved into the expression they group. That is what lets the printer stay
// a plain walk: it writes the parentheses the source had, instead of working
// out where they would be needed to mean the same thing.
//
// A comma turns the same brackets into a tuple, which is why one rule reads
// both: what they are is only known after the first expression.
//
//   parenthesis_or_tuple := '(' expression (',' expression?)* ')'
u32 Parser::parse_parenthesis_or_tuple() {
    u32 token = current_token;

    expect_on_same_line(TK_LEFT_PARENTHESIS);
    open_brackets++;

    u32 expression = parse_expression();

    if (!lookahead_on_same_line(TK_COMMA)) {
        expect_on_same_line(TK_RIGHT_PARENTHESIS);
        open_brackets--;

        return builder.make_parenthesis(token, expression);
    }

    u32 node = builder.make_tuple(token);
    u32 last = builder.add_child(node, 0, expression);

    // a trailing comma is allowed, which is what the second test is for. Every
    // bracketed list here takes one — the argument list of a call is the only
    // one that does not, which is where the reference left it
    while (match_on_same_line(TK_COMMA)) {
        if (lookahead_on_same_line(TK_RIGHT_PARENTHESIS)) {
            break;
        }

        last = builder.add_child(node, last, parse_expression());
    }

    expect_on_same_line(TK_RIGHT_PARENTHESIS);
    open_brackets--;

    return node;
}

//   list := '[' (expression (',' expression?)*)? ']'
u32 Parser::parse_list() {
    u32 token = current_token;

    expect_on_same_line(TK_LEFT_SQUARE_BRACKET);
    open_brackets++;

    u32 node = builder.make_list(token);

    if (!lookahead_on_same_line(TK_RIGHT_SQUARE_BRACKET)) {
        u32 last = builder.add_child(node, 0, parse_expression());

        while (match_on_same_line(TK_COMMA)) {
            if (lookahead_on_same_line(TK_RIGHT_SQUARE_BRACKET)) {
                break;
            }

            last = builder.add_child(node, last, parse_expression());
        }
    }

    expect_on_same_line(TK_RIGHT_SQUARE_BRACKET);
    open_brackets--;

    return node;
}

// The same brackets write an array and a hash, and which one it is shows only
// after the first element: an identifier followed by a ':' opens a hash,
// anything else is an array. That is the reference's test, and it is also why
// the space in '{key: value}' is a language rule — glued, 'key:value' scans as
// an identifier followed by a symbol and never reaches here as a pair.
//
// An empty '{}' is an array with nothing in it. The reference gave back nothing
// at all for that, which cannot be written back as source.
//
//   array_or_hash := '{' (hash | expression (',' expression?)*)? '}'
u32 Parser::parse_array_or_hash() {
    u32 token = current_token;
    u32 node;

    expect_on_same_line(TK_LEFT_CURLY_BRACKET);
    open_brackets++;

    if (match_on_same_line(TK_RIGHT_CURLY_BRACKET)) {
        open_brackets--;

        return builder.make_array(token);
    }

    u32 first = parse_expression();

    // the hash reads its own closing brace, since it is what ends its last
    // pair, and the count comes back down here either way
    if (kind_of(first) == AST_IDENTIFIER
        && lookahead_on_same_line(TK_COLON)) {
        node = parse_hash(token, first);
        open_brackets--;

        return node;
    }

    node = builder.make_array(token);

    u32 last = builder.add_child(node, 0, first);

    while (match_on_same_line(TK_COMMA)) {
        if (lookahead_on_same_line(TK_RIGHT_CURLY_BRACKET)) {
            break;
        }

        last = builder.add_child(node, last, parse_expression());
    }

    expect_on_same_line(TK_RIGHT_CURLY_BRACKET);
    open_brackets--;

    return node;
}

// the first key is already read, since it is what told a hash from an array
//
//   hash := identifier ':' expression (',' identifier ':' expression)*
u32 Parser::parse_hash(u32 token, u32 key) {
    u32 node = builder.make_hash(token);
    u32 last = 0;

    while (true) {
        u32 colon = current_token;

        expect_on_same_line(TK_COLON);

        u32 value = parse_expression();

        last = builder.add_child(node, last,
                                 builder.make_hash_pair(colon, key, value));

        if (!match_on_same_line(TK_COMMA)) {
            break;
        }

        // a trailing comma, like a list, an array and a tuple take
        if (lookahead_on_same_line(TK_RIGHT_CURLY_BRACKET)) {
            break;
        }

        key = parse_identifier();
    }

    expect_on_same_line(TK_RIGHT_CURLY_BRACKET);

    return node;
}

// A closure carries its own parameters, an optional return type after '->' and
// a body between braces. The parameters are not written with '@' and their type
// is optional, which is what keeps them from being the 'def' parameter rule.
//
//   closure := ('||' | '|' (closure_parameter (',' closure_parameter)*)? '|')
//              ('->' type)? '{' braced_block '}'
u32 Parser::parse_closure() {
    u32 token = current_token;

    // '||' is one token, so a closure with no parameters arrives with both
    // pipes glued together. Splitting a compound token by hand is the same
    // move nested generics need for the '>>' that closes them
    bool empty_parameters = match_on_same_line(TK_LOGICAL_OR);

    if (!empty_parameters) {
        expect_on_same_line(TK_BITWISE_OR);
    }

    u32 node = builder.make_closure(token);
    u32 last = 0;

    if (!empty_parameters) {
        if (!lookahead_on_same_line(TK_BITWISE_OR)) {
            last = builder.add_child(node, last, parse_closure_parameter());

            while (match_on_same_line(TK_COMMA)) {
                last = builder.add_child(node, last, parse_closure_parameter());
            }
        }

        expect_on_same_line(TK_BITWISE_OR);
    }

    if (match_on_same_line(TK_ARROW)) {
        last = builder.add_child(node, last,
                                 builder.make_closure_return_type(parse_type()));
    }

    expect_on_same_line(TK_LEFT_CURLY_BRACKET);

    builder.add_child(node, last, parse_braced_block());

    // the brace that closes it may be lines below, so no line rule here
    expect(TK_RIGHT_CURLY_BRACKET);

    return node;
}

//   closure_parameter := identifier (':' type)?
u32 Parser::parse_closure_parameter() {
    u32 name = parse_binding_name();
    u32 type = parse_binding_type();

    return builder.make_closure_parameter(name, type);
}

// a literal is written back exactly as it was read — the scanner's lexeme keeps
// the quotes of a string, the ':' of a symbol and the '_' separators of a
// number — so the node needs nothing but the token
//
//   literal := integer | float | string | char | symbol | 'true' | 'false'
u32 Parser::parse_literal() {
    TokenKind token_kind = current().get_kind();
    AstNodeKind kind = literal_kind(token_kind);

    if (kind == AST_UNKNOWN) {
        return 0;
    }

    if (!match_on_same_line(token_kind)) {
        return 0;
    }

    return builder.make_literal(kind, matched);
}

// '::' binds tighter than anything, so it is resolved here, below every
// operator. It takes exactly one alias: 'std::io::println' is not a chain, it
// is 'std::io' with a '::println' left over, and the statement loop reports
// that leftover as a declaration that opens nothing.
//
// A plain identifier is given back as itself rather than wrapped, so a name
// with no qualification costs no node.
//
//   scope := '::' identifier
//          | identifier ('::' identifier)?
u32 Parser::parse_scope() {
    // '::name' looks the name up outside every alias, so it has no left side.
    // The builder writes that as a 0 alias
    if (match_on_same_line(TK_SCOPE)) {
        u32 oper = matched;
        u32 name = parse_identifier();

        return builder.make_scope(oper, 0, name);
    }

    u32 alias = parse_identifier();

    if (!match_on_same_line(TK_SCOPE)) {
        return alias;
    }

    u32 oper = matched;
    u32 name = parse_identifier();

    return builder.make_scope(oper, alias, name);
}

// an identifier that is not there gives back 0, which add_child skips: an
// error leaves the tree shorter, never with a hole in it
u32 Parser::parse_identifier() {
    if (expect_on_same_line(TK_IDENTIFIER)) {
        return builder.make_identifier(matched);
    }

    return 0;
}

bool Parser::lookahead(TokenKind kind) {
    return !panic && current().get_kind() == kind;
}

bool Parser::match(TokenKind kind) {
    if (!lookahead(kind)) {
        return false;
    }

    matched = current_token;
    advance();

    return true;
}

bool Parser::expect(TokenKind kind) {
    if (match(kind)) {
        return true;
    }

    error_expected(kind, false);

    return false;
}

// the token that opens a statement always begins a line, so the rule cannot be
// applied to it — it is what starts the line the rest has to stay on. Every
// other rule reads its first token with the plain 'expect', which is why this
// only started to matter with the function body, an expression that opens a
// line of its own
// A statement lives on one line, and inside a bracket it does not. Everything
// between a '(', a '[' or a '{' and the token that closes it is one expression
// however many lines it is spread over, because the bracket says where it ends
// and the line rule has nothing left to say. It is Python's implicit line
// joining and it is the same reason: the rule exists to find the end of a
// statement, and here the end is already written down.
//
// A closure's braced block is not one of those brackets. Its contents are
// statements and a statement is found by its line, so parse_body puts the rule
// back for as long as the block lasts.
bool Parser::on_same_line() {
    return open_brackets > 0
        || current_token == statement_first_token
        || !current().get_newline_before();
}

void Parser::begin_statement() {
    statement_first_token = current_token;

    // and the line rule is in force again, whatever an unclosed bracket in the
    // statement before left behind. A statement that failed inside a bracket
    // never reaches the token that would have closed it, so the count is only
    // balanced on the path where nothing went wrong -- and this is the anchor
    // that makes not balancing it harmless
    open_brackets = 0;
}

// nothing at all between the two tokens: no space, no comment, no line break
bool Parser::glued_to_previous() {
    if (current_token == 0) {
        return false;
    }

    Token& previous = tokens->get_token(current_token - 1);

    return previous.get_offset() + previous.get_length() == current().get_offset();
}

void Parser::indent(u32 indentation) {
    indentation_stack.push_back(indentation);
}

void Parser::dedent() {
    // the 0 the stack is born with is not a level anyone pushed, so it stays
    if (indentation_stack.size() > 1) {
        indentation_stack.pop_back();
    }
}

// inert in panic like every other predicate, so the loops that read a block
// stop by themselves instead of each one checking the flag
bool Parser::is_indented() {
    return !panic
        && current().get_kind() != TK_EOF
        && indentation_of_current_line() > indentation_stack.back();
}

bool Parser::lookahead_on_same_line(TokenKind kind) {
    return on_same_line() && lookahead(kind);
}

bool Parser::match_on_same_line(TokenKind kind) {
    return on_same_line() && match(kind);
}

bool Parser::expect_on_same_line(TokenKind kind) {
    if (match_on_same_line(kind)) {
        return true;
    }

    error_expected(kind, true);

    return false;
}

// freezes while panic is set, so that everything after the error looks at the
// same token and no second error is invented out of a position that moved
void Parser::advance() {
    if (panic) {
        return;
    }

    if (current().get_kind() != TK_EOF) {
        ++current_token;
    }
}

Token& Parser::current() {
    return tokens->get_token(current_token);
}

// what the parser just built, which is how an array is told from a hash. The
// sentinel at 0 answers AST_UNKNOWN, so an expression that failed takes the
// path that reports the next error rather than a special case here
AstNodeKind Parser::kind_of(u32 node) {
    return module->get_ast()->get_node(node)->get_kind();
}

u32 Parser::indentation_of_current_line() {
    return current().get_whitespace();
}

// 'same_line' says whether what was expected had to be on the line already
// being read. Only then does a token that opens a new line mean the line ran
// out; the first token of a statement always opens one
void Parser::error_expected(TokenKind kind, bool same_line) {
    error_found(describe(kind), same_line);
}

// same thing for a rule that wanted a whole category rather than one kind,
// which is what the statement dispatch has to report
void Parser::error_found(const std::string& expectation, bool same_line) {
    // already reporting one error for this statement: the ones that follow are
    // consequences of it, not news
    if (panic) {
        return;
    }

    panic = true;

    Token& token = current();
    bool at_end_of_file = token.get_kind() == TK_EOF;

    // the line ran out exactly when the line rule says it did, which is not the
    // same as 'this token opens a line': the token that opens the statement
    // being read is the one exception, and blaming the line above it would
    // point the caret at somebody else's code
    bool at_end_of_line = same_line && !on_same_line();

    // what was needed never came: pointing at the first token of the next line
    // would blame a line that has nothing to do with it, so the caret goes just
    // past the last token of this one
    if ((at_end_of_file || at_end_of_line) && current_token > 0) {
        Token& previous = tokens->get_token(current_token - 1);
        std::string found = at_end_of_file
            ? "the end of the file"
            : "the end of the line";

        logger->error(previous.get_offset() + previous.get_length(), 0,
                      "expected " + expectation + ", found " + found);
        return;
    }

    logger->error(token.get_offset(), token.get_length(),
                  "expected " + expectation + ", found '"
                  + std::string(module->get_token_value(current_token)) + "'");
}

// the second half of the recovery: throw away what is left of the line that
// failed, and the lines it opened. Everything indented deeper than the line the
// statement started on belonged to it
// for a rule that has a token in front of it and something to say about it,
// rather than a kind it was hoping for
void Parser::error_at_current(const std::string& message) {
    if (panic) {
        return;
    }

    panic = true;

    Token& token = current();

    logger->error(token.get_offset(), token.get_length(), message);
}

// the flag without a message. What it says is 'this statement cannot be kept',
// which is not always the same as 'here is an error': a block left empty by a
// statement that already failed has nothing new to report, but keeping it would
// put something in the tree that is not writable back as source
void Parser::poison() {
    panic = true;
}

// The second half of the recovery: throw away what is left of the line that
// failed, and the lines it opened. Everything indented deeper than the line the
// statement started on belonged to it.
void Parser::synchronize(u32 statement_indentation, u32 statement_start,
                         bool braced) {
    panic = false;

    // a statement that failed without consuming a single token has to give one
    // up, or the loop calling this would meet the same error forever. One that
    // did consume tokens is already past them, and if it stopped at the start
    // of a line then that line is a fresh statement: taking it would make one
    // bad statement swallow a good one
    if (current_token == statement_start) {
        skip_to_next_line(braced);
    } else if (!current().get_newline_before()) {
        skip_to_next_line(braced);
    }

    while (!at_block_end(braced)
           && indentation_of_current_line() > statement_indentation) {
        skip_to_next_line(braced);
    }
}

// Moves at least one token and stops on the first of the next line — unless the
// block is a braced one and its brace is what comes next, because skipping past
// that would leave the closure looking unclosed and turn one mistake into two
// errors.
//
// Stopping without consuming cannot spin here: the loop that calls this ends on
// the same brace.
void Parser::skip_to_next_line(bool braced) {
    if (at_block_end(braced)) {
        return;
    }

    advance();

    while (!at_block_end(braced) && !current().get_newline_before()) {
        advance();
    }
}

// where a recovery has to stop. A braced block ends at its brace, not at the
// end of a line
bool Parser::at_block_end(bool braced) {
    if (current().get_kind() == TK_EOF) {
        return true;
    }

    return braced && lookahead(TK_RIGHT_CURLY_BRACKET);
}
