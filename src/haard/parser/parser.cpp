#include <haard/parser/parser.h>

using namespace haard;

// what to call a token kind inside a message. Only the kinds the parser asks
// for need one, so this grows with the grammar
static std::string describe(TokenKind kind) {
    switch (kind) {
        case TK_IMPORT: return "'import'";
        case TK_DEF: return "'def'";
        case TK_AT: return "'@'";
        case TK_COLON: return "':'";
        case TK_COMMA: return "','";
        case TK_GREATER_THAN: return "'>'";
        case TK_LET: return "'let'";
        case TK_CONST: return "'const'";
        case TK_AS: return "'as'";
        case TK_DOT: return "'.'";
        case TK_LEFT_PARENTHESIS: return "'('";
        case TK_TEMPLATE_STRING_END: return "the end of the template string";
        case TK_INTERPOLATION_END: return "'}'";
        case TK_RIGHT_PARENTHESIS: return "')'";
        case TK_IDENTIFIER: return "an identifier";
        case TK_EOF: return "the end of the file";
        default: break;
    }

    return "a different token";
}

// the ast kind a literal token becomes, or AST_UNKNOWN when the token is not a
// literal at all. They all become the same shape of node — a kind and the token
// it was written as — so what separates them is only this table
static AstNodeKind literal_kind(TokenKind kind) {
    switch (kind) {
        case TK_INTEGER_LITERAL: return AST_INTEGER_LITERAL;
        case TK_FLOAT_LITERAL: return AST_FLOAT_LITERAL;
        case TK_STRING_LITERAL: return AST_STRING_LITERAL;
        case TK_CHAR_LITERAL: return AST_CHAR_LITERAL;
        case TK_SYMBOL_LITERAL: return AST_SYMBOL_LITERAL;
        case TK_TRUE: return AST_TRUE;
        case TK_FALSE: return AST_FALSE;
        default: break;
    }

    return AST_UNKNOWN;
}

Parser::Parser() {
    context = nullptr;
    tokens = nullptr;
    logger = nullptr;
    current_token = 0;
    matched = 0;
    statement_first_token = 0;
    panic = false;
    indentation_stack.push_back(0);
}

void Parser::set_context(Context* context) {
    this->context = context;
    this->tokens = context->get_tokens();
    this->logger = context->get_logger();

    builder.set_ast(context->get_ast());
}

u32 Parser::parse() {
    current_token = 0;
    matched = 0;
    statement_first_token = 0;
    panic = false;

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
            synchronize(indentation, start);
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

//   import_path := identifier ('.' identifier)*
u32 Parser::parse_import_path() {
    u32 node = builder.make_import_path();
    u32 last = builder.add_child(node, 0, parse_import_path_segment());

    // in panic the match answers false, so a broken path stops here instead of
    // reporting one error per segment
    while (match_on_same_line(TK_DOT)) {
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
    u32 last = builder.add_child(node, 0, parse_binding_name());

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
//   block := statement*
u32 Parser::parse_block(u32 header_indentation) {
    u32 node = builder.make_block();
    u32 last = 0;

    indent(header_indentation);

    // in panic this is inert, so a header whose condition failed reads no
    // block at all and the error travels out to the statement that can recover
    // from it, instead of being recovered inside a block that never opened
    while (is_indented()) {
        u32 indentation = indentation_of_current_line();
        u32 start = current_token;

        begin_statement();

        u32 child = parse_statement();

        if (panic) {
            synchronize(indentation, start);
            continue;
        }

        last = builder.add_child(node, last, child);
    }

    dedent();

    return node;
}

// an expression is what is left when no keyword opens the line, so it needs no
// lookahead of its own
//
//   statement := if | while | expression
u32 Parser::parse_statement() {
    if (lookahead(TK_IF)) {
        return parse_if();
    }

    if (lookahead(TK_WHILE)) {
        return parse_while();
    }

    return parse_expression();
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

//   param := '@' identifier ':' type
u32 Parser::parse_param() {
    u32 token = current_token;

    begin_statement();
    expect(TK_AT);

    u32 name = parse_binding_name();
    u32 type = parse_param_type();

    return builder.make_param(token, name, type);
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
    u32 name = parse_binding_name();
    u32 type = parse_binding_type();
    u32 expression = parse_binding_expression();

    return builder.make_binding(name, type, expression);
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

//   type := identifier
u32 Parser::parse_type() {
    return parse_identifier();
}

u32 Parser::parse_expression() {
    return parse_arith_expression();
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
//   term_expression := primary_expression (('*' | '/' | '%') primary_expression)*
u32 Parser::parse_term_expression() {
    u32 node = parse_primary_expression();

    while (true) {
        AstNodeKind kind;

        if (match_on_same_line(TK_TIMES)) {
            kind = AST_TIMES;
        } else if (match_on_same_line(TK_DIVISION)) {
            kind = AST_DIVISION;
        } else if (match_on_same_line(TK_MODULO)) {
            kind = AST_MODULO;
        } else {
            break;
        }

        u32 oper = matched;
        u32 right = parse_primary_expression();

        node = builder.make_binary_operator(kind, oper, node, right);
    }

    return node;
}

//   primary_expression := parenthesis | literal | scope
u32 Parser::parse_primary_expression() {
    if (lookahead_on_same_line(TK_LEFT_PARENTHESIS)) {
        return parse_parenthesis();
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
        return parse_scope();
    }

    // reported here rather than left to the scope rule, which would say it
    // expected an identifier: an identifier is one of the five things allowed
    // in this position, not the only one
    error_found("an expression", true);

    return 0;
}

// the parentheses are kept in the tree as a node of their own, rather than
// dissolved into the expression they group. That is what lets the printer stay
// a plain walk: it writes the parentheses the source had, instead of working
// out where they would be needed to mean the same thing
//
//   parenthesis := '(' expression ')'
u32 Parser::parse_parenthesis() {
    u32 token = current_token;

    expect_on_same_line(TK_LEFT_PARENTHESIS);

    u32 expression = parse_expression();

    expect_on_same_line(TK_RIGHT_PARENTHESIS);

    return builder.make_parenthesis(token, expression);
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
bool Parser::on_same_line() {
    return current_token == statement_first_token
        || !current().get_newline_before();
}

void Parser::begin_statement() {
    statement_first_token = current_token;
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
                  + std::string(context->get_token_value(current_token)) + "'");
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

void Parser::synchronize(u32 statement_indentation, u32 statement_start) {
    panic = false;

    // a statement that failed without consuming a single token has to give one
    // up, or the loop calling this would meet the same error forever. One that
    // did consume tokens is already past them, and if it stopped at the start
    // of a line then that line is a fresh statement: taking it would make one
    // bad statement swallow a good one
    if (current_token == statement_start) {
        skip_to_next_line();
    } else if (!current().get_newline_before()) {
        skip_to_next_line();
    }

    while (!lookahead(TK_EOF)
           && indentation_of_current_line() > statement_indentation) {
        skip_to_next_line();
    }
}

// moves at least one token and stops on the first of the next line
void Parser::skip_to_next_line() {
    advance();

    while (!lookahead(TK_EOF) && !current().get_newline_before()) {
        advance();
    }
}
