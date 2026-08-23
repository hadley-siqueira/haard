#ifndef HAARD_AST_BUILDER_H
#define HAARD_AST_BUILDER_H

#include <haard/ast/ast.h>

namespace haard {
    // Builds the nodes so that the parser does not have to.
    //
    // A node of fixed shape is made in one call, with the children it already
    // has:
    //
    //     return builder->make_binary_operator(AST_PLUS, oper, left, right);
    //
    // A node that holds a list is born empty and grows, which is how the parser
    // reads a list in the first place. The caller carries the last child from
    // one round to the next:
    //
    //     u32 node = builder->make_module();
    //     u32 last = 0;
    //
    //     while (...) {
    //         last = builder->add_child(node, last, parse_statement());
    //     }
    //
    // Carrying it is not tree building, it is a loop cursor: writing the links
    // stays in here. And a 'last' of 0 means 'I do not know', not 'there is
    // nothing yet', so forgetting the assignment above makes the append slow,
    // never wrong.
    //
    // It keeps no state of its own between calls: no stack of open nodes, no
    // current parent. That is on purpose. The parser enters panic mode and
    // leaves functions early, and a builder with an open node would need every
    // one of those paths to close it, which is the kind of bookkeeping the
    // poisoned parser design exists to avoid.
    //
    // A child of 0 means 'not there' and is skipped, so an optional part needs
    // no special case at the call site.
    class AstBuilder {
        public:
            AstBuilder();

        public:
            void set_ast(Ast* ast);

        public:
            // appends and gives back the new last child, to be passed in again
            u32 add_child(u32 parent, u32 last, u32 child);

            u32 make_module();

            u32 make_import(u32 token, u32 path, u32 alias);
            u32 make_import_path();
            u32 make_import_path_segment(u32 token);
            u32 make_import_alias(u32 token);

            u32 make_let_declaration(u32 token, u32 binding);
            u32 make_const_declaration(u32 token, u32 binding);

            // a list node: the name, the generic parameters, the return
            // type, the parameters and the body are appended as they are read
            u32 make_function(u32 token);
            u32 make_generic_parameters(u32 token);
            u32 make_function_return_type(u32 child);
            u32 make_function_body(u32 child);

            u32 make_param(u32 token, u32 name, u32 type);
            u32 make_binding(u32 name, u32 type, u32 expression);
            u32 make_binding_name(u32 child);
            u32 make_binding_type(u32 child);
            u32 make_binding_expression(u32 child);

            u32 make_binary_operator(AstNodeKind kind, u32 token, u32 left, u32 right);

            // 'alias' is 0 for the '::name' form, which has no left side
            u32 make_scope(u32 token, u32 alias, u32 name);
            // the '(' is kept as a node of its own, so the printer writes the
            // parentheses the source had instead of working out where they
            // would be needed
            u32 make_parenthesis(u32 token, u32 expression);
            u32 make_identifier(u32 token);

            // every literal is the same node: a kind and the token it was
            // written as, which is also how it is written back
            u32 make_literal(AstNodeKind kind, u32 token);

            // a list node: it is born holding only its quote and grows one
            // chunk or interpolation at a time, the way the parser reads them
            u32 make_template_string(u32 token);
            u32 make_template_string_chunk(u32 token);
            u32 make_interpolation(u32 token, u32 expression);

        private:
            u32 make_node(AstNodeKind kind, u32 token);
            void add_last_child(u32 parent, u32 child);
            u32 last_child_of(u32 parent);

        private:
            Ast* ast;
    };
}

#endif
