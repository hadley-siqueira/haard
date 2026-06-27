#ifndef HAARD_PRETTY_PRINTER_H
#define HAARD_PRETTY_PRINTER_H

#include <haard/context/context.h>
#include <sstream>

namespace haard {
    class PrettyPrinter {
        public:
            PrettyPrinter();

        public:
            void print();

            void print_node(u32 node);
            void print_module(u32 node);

            void print_import(u32 node);
            void print_import_path(u32 node);
            void print_import_path_segment(u32 node);
            void print_import_alias(u32 node);

            void print_let_declaration(u32 node);

            void print_binding(u32 node);
            void print_binding_name(u32 node);
            void print_binding_type(u32 node);
            void print_binding_expression(u32 node);

            void print_identifier(u32 node);

        public:
            void print_children(u32 node);
            void print_children_joined(u32 node, const std::string& sep);

            void print_node_token(u32 node);
            void print_token(u32 token);

            void print_string(const std::string_view& s);

        public:
            void set_context(Context* context);

        private:
            std::stringstream output;
            Context* context;
            Ast* ast;
    };
}

#endif
