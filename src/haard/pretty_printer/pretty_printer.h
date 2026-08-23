#ifndef HAARD_PRETTY_PRINTER_H
#define HAARD_PRETTY_PRINTER_H

#include <haard/context/context.h>
#include <ostream>
#include <sstream>

namespace haard {
    class PrettyPrinter {
        public:
            PrettyPrinter();

        public:
            // false when there is nothing to print, which is what an empty ast
            // means: no phase has built one yet
            bool print(std::ostream& out);

            void print_node(u32 node);
            void print_module(u32 node);

            void print_import(u32 node);
            void print_import_path(u32 node);
            void print_import_path_segment(u32 node);
            void print_import_alias(u32 node);

            void print_let_declaration(u32 node);
            void print_const_declaration(u32 node);

            void print_function(u32 node);
            void print_generic_parameters(u32 node);
            void print_function_return_type(u32 node);

            void print_block(u32 node);
            void print_pass(u32 node);
            void print_if(u32 node);
            void print_elif(u32 node);
            void print_else(u32 node);
            void print_while(u32 node);

            void print_return(u32 node);
            void print_break(u32 node);
            void print_continue(u32 node);
            void print_yield(u32 node);
            void print_goto(u32 node);

            // the keyword, and the expression after it only when there is one
            void print_jump(u32 node, const std::string& keyword);

            // the shape 'if', 'elif', 'else' and 'while' share: a header line
            // ending in ':' and a block of lines under it
            void print_conditional(u32 node, const std::string& keyword);

            void print_param(u32 node);
            void print_binding(u32 node);
            void print_binding_name(u32 node);
            void print_binding_type(u32 node);
            void print_binding_expression(u32 node);

            void print_plus_expression(u32 node);
            void print_minus_expression(u32 node);
            void print_times_expression(u32 node);
            void print_division_expression(u32 node);
            void print_modulo_expression(u32 node);
            void print_scope(u32 node);
            void print_parenthesis(u32 node);
            void print_identifier(u32 node);

            // every literal prints the same way: the lexeme it was written as
            void print_literal(u32 node);

            void print_template_string(u32 node);
            void print_template_string_chunk(u32 node);
            void print_interpolation(u32 node);

        public:
            void print_children(u32 node);
            void print_children_joined(u32 node, const std::string& sep);

            void print_node_token(u32 node);
            void print_token(u32 token);

            void print_string(const std::string_view& s);

            // a line break followed by the current indentation. Four spaces a
            // level: the block rule accepts any depth, so what comes out is
            // one depth rather than the one that went in
            void print_new_line();

        public:
            void set_context(Context* context);

        private:
            std::stringstream output;
            Context* context;
            Ast* ast;
            u32 indentation;
    };
}

#endif
