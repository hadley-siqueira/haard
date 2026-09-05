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

            // A subtree copied node for node into the same tree, sharing the
            // tokens it was written with. Record 0002 instantiates a generic
            // by cloning its declaration, and every phase after that reads the
            // clone the way it reads anything the parser built -- so the copy
            // has to be a real tree and not a view of one.
            //
            // The copy's root gets no sibling: it is about to be given one by
            // whoever appends it
            u32 clone(u32 node);

            u32 make_module();

            u32 make_import(u32 token, u32 path, u32 alias);
            u32 make_import_path();
            u32 make_import_path_segment(u32 token);
            u32 make_import_alias(u32 token);
            u32 make_import_all(u32 token);

            u32 make_let_declaration(u32 token, u32 binding);
            u32 make_const_declaration(u32 token, u32 binding);

            // a list node: the name, the generic parameters, the return
            // type, the parameters and the body are appended as they are read
            // class, struct, enum and union are one shape with four words,
            // so one maker takes the kind
            u32 make_type_declaration(AstNodeKind kind, u32 token);
            u32 make_super_type(u32 token, u32 type);
            u32 make_type_body();
            u32 make_field(u32 name, u32 type, u32 value);

            u32 make_function(u32 token);
            u32 make_generic_parameters(u32 token);
            u32 make_function_return_type(u32 child);

            u32 make_block();

            // the word that fills an empty block. It is a node rather than
            // nothing, because a block with no children is not writable back
            u32 make_pass(u32 token);

            // the header keyword's token, then the condition, the block and —
            // for an 'if' — the elif and else that follow it, as siblings
            u32 make_if(u32 token);
            u32 make_elif(u32 token);
            u32 make_else(u32 token);
            u32 make_while(u32 token);

            // A 'for' is three loops written with one word, and which one it
            // is shows only after its head has been read. So the head is built
            // first, on its own, and handed to whichever of the two takes it.
            u32 make_for(u32 token);
            u32 make_for_each(u32 token);
            u32 make_for_head();
            u32 make_for_condition();
            u32 make_for_increment();

            // return, break, continue, yield and goto are one shape: the
            // keyword, and an expression only when one was written after it
            u32 make_jump(AstNodeKind kind, u32 token, u32 expression);

            // the label a 'goto' jumps to
            u32 make_label(u32 token, u32 name);

            u32 make_param(u32 token, u32 name, u32 type, u32 value);
            u32 make_binding(u32 name, u32 type, u32 expression);
            u32 make_binding_name(u32 child);
            u32 make_binding_type(u32 child);
            u32 make_binding_expression(u32 child);

            u32 make_binary_operator(AstNodeKind kind, u32 token, u32 left, u32 right);

            // 'alias' is 0 for the '::name' form, which has no left side
            u32 make_scope(u32 token, u32 alias, u32 name);

            // the postfix operators, all of them left associative: what they
            // are applied to is their first child
            u32 make_index(u32 token, u32 target, u32 subscript);
            u32 make_call(u32 token, u32 callee, u32 arguments);
            u32 make_arguments(u32 token);
            u32 make_unary_operator(AstNodeKind kind, u32 token, u32 operand);

            u32 make_new(u32 token, u32 type, u32 arguments);
            u32 make_sizeof(u32 token, u32 expression);
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
            // 'this' and the four bracketed forms. The three that hold a
            // list are born empty and grow, like every other list node here
            u32 make_this(u32 token);
            u32 make_list(u32 token);
            u32 make_array(u32 token);
            u32 make_hash(u32 token);
            u32 make_hash_pair(u32 token, u32 key, u32 value);
            u32 make_tuple(u32 token);

            u32 make_closure(u32 token);
            u32 make_closure_parameter(u32 name, u32 type);
            u32 make_closure_return_type(u32 child);

            // The types. A named type wraps the name so that a walk can tell
            // a type reference from a variable one without looking at its
            // parent; the three list shaped ones are born empty and grow.
            u32 make_named_type(u32 name, u32 arguments);

            // one kind for the ten sized numeric types: which one it is, is
            // the token, exactly as it is for a literal
            u32 make_builtin_type(u32 token);

            // a name in an expression carrying its type arguments, which is
            // how a generic function is called with them written out
            u32 make_generic_name(u32 name, u32 arguments);
            u32 make_generic_arguments(u32 token);
            u32 make_pointer_type(u32 token, u32 type);
            u32 make_reference_type(u32 token, u32 type);
            u32 make_array_type(u32 token, u32 type, u32 size);
            u32 make_list_type(u32 token, u32 type);
            u32 make_hash_type(u32 token, u32 key, u32 value);
            u32 make_tuple_type(u32 token);
            u32 make_function_type(u32 token);

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
