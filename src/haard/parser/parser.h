#ifndef HAARD_PARSER_H
#define HAARD_PARSER_H

#include <haard/ast/ast_builder.h>
#include <haard/context/context.h>
#include <vector>

namespace haard {
    // Recursive descent over the token stream the scanner left in the Context.
    //
    // Error handling is the poisoned parser: reporting an error sets 'panic',
    // and while it is set every primitive here goes inert — lookahead answers
    // false, advance freezes, expect reports nothing new. A loop written as
    // 'while (match(...))' therefore stops by itself and a branch written as
    // 'if (lookahead(...))' is not taken, so no parse function has to check
    // whether the one it called failed.
    //
    // The only places that look at 'panic' are the statement loops that
    // recover from it, by calling synchronize. There is one so far, in
    // parse_module.
    class Parser {
        public:
            Parser();

        public:
            void set_context(Context* context);

            // parses the tokens in the context, gives back the root node
            u32 parse();

        private:
            u32 parse_module();
            u32 parse_declaration();

            u32 parse_import();
            u32 parse_import_path();
            u32 parse_import_path_segment();
            u32 parse_import_alias();

            u32 parse_let_declaration();
            u32 parse_const_declaration();

            u32 parse_function();
            u32 parse_generic_parameters();
            u32 parse_function_return_type();
            u32 parse_param();
            u32 parse_param_type();

            // the block owns the indentation stack: it is the only rule that
            // pushes a level, and it is the second of the two recovery points
            u32 parse_block(u32 header_indentation);
            u32 parse_pass();
            u32 parse_statement();
            u32 parse_if();
            u32 parse_elif();
            u32 parse_else();
            u32 parse_while();
            u32 parse_jump(TokenKind keyword, AstNodeKind kind);
            u32 parse_conditional(u32 node, u32 header_indentation);

            u32 parse_binding();
            u32 parse_binding_name();
            u32 parse_binding_type();
            u32 parse_binding_expression();

            u32 parse_type();

            u32 parse_expression();
            u32 parse_arith_expression();
            u32 parse_term_expression();
            u32 parse_primary_expression();
            u32 parse_parenthesis();
            u32 parse_literal();
            u32 parse_template_string();
            u32 parse_template_string_chunk();
            u32 parse_interpolation();
            u32 parse_scope();

            u32 parse_identifier();

        private:
            // navigation. All of it inert while panic is set
            bool lookahead(TokenKind kind);
            bool match(TokenKind kind);
            bool expect(TokenKind kind);
            void advance();

            // a statement lives on one line, so everything after the word that
            // opens it has to be found before the line ends
            bool on_same_line();
            bool lookahead_on_same_line(TokenKind kind);
            bool match_on_same_line(TokenKind kind);
            bool expect_on_same_line(TokenKind kind);

            Token& current();
            u32 indentation_of_current_line();

            // two tokens are glued when nothing at all sits between them, which
            // is how the language separates 'Foo<T>' from 'a < b'
            bool glued_to_previous();

            // the block rule: the header line pushes its own indentation and
            // everything deeper belongs to the body. Aligned or not — a block
            // is permissive on purpose
            void indent(u32 indentation);
            void dedent();
            bool is_indented();

            // the token that opens the statement being read. It is the one
            // token the line rule cannot be applied to, because it is what
            // starts the line everything after it has to stay on
            void begin_statement();

            void error_expected(TokenKind kind, bool same_line);
            void error_found(const std::string& expectation, bool same_line);
            void error_at_current(const std::string& message);

            // the statement cannot be kept and the reason is already reported
            void poison();
            void synchronize(u32 statement_indentation, u32 statement_start);
            void skip_to_next_line();

        private:
            Context* context;
            TokenStream* tokens;
            Logger* logger;
            AstBuilder builder;

            u32 current_token;
            u32 matched;
            u32 statement_first_token;
            bool panic;

            // starts holding a 0, so asking for the top is always answerable.
            // The old compiler's started empty and reading it was undefined
            std::vector<u32> indentation_stack;
    };
}

#endif
