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
            void print_import_all(u32 node);

            void print_let_declaration(u32 node);
            void print_const_declaration(u32 node);

            void print_class(u32 node);
            void print_struct(u32 node);
            void print_enum(u32 node);
            void print_union(u32 node);
            void print_super_type(u32 node);
            void print_field(u32 node);

            // the shape the four of them share: a header line ending in ':'
            // and a body of members under it
            void print_type_declaration(u32 node, const std::string& keyword);

            void print_function(u32 node);
            void print_generic_parameters(u32 node);
            void print_function_return_type(u32 node);

            void print_block(u32 node);
            void print_pass(u32 node);
            void print_if(u32 node);
            void print_elif(u32 node);
            void print_else(u32 node);
            void print_while(u32 node);

            // the three loops share a printer: what changes is which parts
            // are there, and each part writes its own separator
            void print_for(u32 node);
            void print_for_head(u32 node);
            void print_for_condition(u32 node);
            void print_for_increment(u32 node);

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
            void print_integer_division_expression(u32 node);
            void print_modulo_expression(u32 node);
            void print_assignment_expression(u32 node);
            void print_cast_expression(u32 node);
            void print_logical_expression(u32 node);
            void print_comparison_expression(u32 node);
            void print_range_expression(u32 node);
            void print_not_in_expression(u32 node);

            // 'and' and '&&' mean the same thing, so which was written lives
            // in the token, the way a template string's quote does
            void print_binary_from_token(u32 node);

            void print_power_expression(u32 node);
            void print_bitwise_or_expression(u32 node);
            void print_bitwise_xor_expression(u32 node);
            void print_bitwise_and_expression(u32 node);
            void print_left_shift_expression(u32 node);
            void print_right_shift_expression(u32 node);
            void print_unsigned_right_shift_expression(u32 node);

            void print_logical_not(u32 node);
            void print_logical_not_operator(u32 node);
            void print_address_of(u32 node);
            void print_dereference(u32 node);
            void print_bitwise_not(u32 node);
            void print_unary_minus(u32 node);
            void print_unary_plus(u32 node);
            void print_pre_increment(u32 node);
            void print_pre_decrement(u32 node);
            void print_new(u32 node);
            void print_delete(u32 node);
            void print_delete_array(u32 node);
            void print_sizeof(u32 node);

            // an operator written in front of what it applies to
            void print_prefix(u32 node, const std::string& oper);

            void print_scope(u32 node);

            void print_dot(u32 node);
            void print_arrow(u32 node);
            void print_index(u32 node);
            void print_call(u32 node);
            void print_arguments(u32 node);
            void print_post_increment(u32 node);
            void print_post_decrement(u32 node);

            // an operator written after what it applies to
            void print_postfix(u32 node, const std::string& oper);
            void print_parenthesis(u32 node);
            void print_this(u32 node);
            void print_list(u32 node);
            void print_array(u32 node);
            void print_hash(u32 node);
            void print_hash_pair(u32 node);
            void print_tuple(u32 node);

            void print_closure(u32 node);
            void print_closure_parameter(u32 node);
            void print_closure_return_type(u32 node);

            void print_named_type(u32 node);
            void print_builtin_type(u32 node);
            void print_generic_name(u32 node);
            void print_generic_arguments(u32 node);
            void print_pointer_type(u32 node);
            void print_reference_type(u32 node);
            void print_array_type(u32 node);
            void print_list_type(u32 node);
            void print_hash_type(u32 node);
            void print_tuple_type(u32 node);
            void print_function_type(u32 node);
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

            // the last character written, so that two operators written next
            // to each other cannot be read back as one
            char last_character;

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
