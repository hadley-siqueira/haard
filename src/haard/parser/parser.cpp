#include <haard/parser/parser.h>

using namespace haard;

// what to call a token kind inside a message. Only the kinds the parser asks
// for need one, so this grows with the grammar
static std::string describe(TokenKind kind) {
    switch (kind) {
        case TK_IMPORT: return "'import'";
        case TK_AS: return "'as'";
        case TK_DOT: return "'.'";
        case TK_IDENTIFIER: return "an identifier";
        case TK_EOF: return "the end of the file";
        default: break;
    }

    return "a different token";
}

Parser::Parser() {
    context = nullptr;
    tokens = nullptr;
    logger = nullptr;
    current_token = 0;
    matched = 0;
    panic = false;
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
    panic = false;

    return parse_module();
}

//   module := import*
u32 Parser::parse_module() {
    u32 node = builder.make_module();
    u32 last = 0;

    while (!lookahead(TK_EOF)) {
        // the line the statement starts on, so that a failed one can take the
        // lines it opened down with it
        u32 indentation = indentation_of_current_line();
        u32 child = parse_import();

        // one of the two places that know about panic mode: the recovery point
        if (panic) {
            synchronize(indentation);
            continue;
        }

        last = builder.add_child(node, last, child);
    }

    return node;
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
    while (match(TK_DOT)) {
        last = builder.add_child(node, last, parse_import_path_segment());
    }

    return node;
}

// a segment that is not there gives back 0, which add_child skips: an error
// leaves the path shorter, never with a hole in it
u32 Parser::parse_import_path_segment() {
    if (expect(TK_IDENTIFIER)) {
        return builder.make_import_path_segment(matched);
    }

    return 0;
}

u32 Parser::parse_import_alias() {
    if (!match(TK_AS)) {
        return 0;
    }

    if (expect(TK_IDENTIFIER)) {
        return builder.make_import_alias(matched);
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

    error_expected(kind);

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

void Parser::error_expected(TokenKind kind) {
    // already reporting one error for this statement: the ones that follow are
    // consequences of it, not news
    if (panic) {
        return;
    }

    panic = true;

    Token& token = current();
    std::string found = token.get_kind() == TK_EOF
        ? describe(TK_EOF)
        : "'" + std::string(context->get_token_value(current_token)) + "'";

    logger->error(token.get_offset(), token.get_length(),
                  "expected " + describe(kind) + ", found " + found);
}

// the second half of the recovery: throw away what is left of the line that
// failed, and the lines it opened. Everything indented deeper than the line the
// statement started on belonged to it
void Parser::synchronize(u32 statement_indentation) {
    panic = false;

    skip_to_next_line();

    while (!lookahead(TK_EOF)
           && indentation_of_current_line() > statement_indentation) {
        skip_to_next_line();
    }
}

// always moves at least one token: without that, the loop that calls
// synchronize would sit on the same error forever
void Parser::skip_to_next_line() {
    advance();

    while (!lookahead(TK_EOF) && !current().get_newline_before()) {
        advance();
    }
}
