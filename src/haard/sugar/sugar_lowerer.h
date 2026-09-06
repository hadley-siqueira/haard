#ifndef HAARD_SUGAR_LOWERER_H
#define HAARD_SUGAR_LOWERER_H

#include <haard/ast/ast_builder.h>
#include <haard/module/module.h>
#include <string>
#include <vector>

namespace haard {
    // The Ast -> Ast pass record 0025 chose for sugar that stops being local,
    // and today it takes apart exactly one thing: a template string.
    //
    // Record 0032. A template string is a String (record 0022), a String is
    // its own builder (record 0023), and what a template string means is a
    // local built by a run of 'append' calls:
    //
    //     out->writeln("lit pixels: ${count(shapes)}")
    //
    //     let __ts0 : String
    //     __ts0.append("lit pixels: ")
    //     __ts0.append(count(shapes))
    //     out->writeln(__ts0)
    //
    // It runs BEFORE the symbols are collected, because the local it writes
    // is a declaration like any other and everything after this point has to
    // see an ordinary tree. Nothing downstream knows a template string
    // existed: overload resolution picks the 'append' for each '${}' by the
    // type of what is in it, which is what makes String's eight overloads
    // carry the whole feature.
    //
    // **Hoisting is the reason this is a pass and not an expression.** The
    // calls have to become statements, and moving them out of the expression
    // they were written in is not always meaning preserving -- so where it is
    // not, this refuses rather than emitting a program that means something
    // else. Loosening it later is additive.
    //
    // The node is rewritten in place into a use of the local, rather than
    // replaced in its parent: it keeps its index and its sibling link, so no
    // walk has to know its parent and nothing else in the tree moves.
    class SugarLowerer {
        public:
            SugarLowerer();

        public:
            void set_module(Module* module);

            // takes apart every template string of this module, logging one
            // error per one that cannot be hoisted
            void lower();

        private:
            // Whether the calls may be lifted out to the statement, and when
            // they may not, which of the three reasons it is. It travels down
            // the walk as a parameter: a template string does not know where
            // it sits, and the walk is the only thing that does
            enum Hoisting {
                HOIST_OK,

                // the condition of a 'while' or the middle of a C shaped
                // 'for': lifting the calls out builds the String once,
                // before the loop, instead of every turn
                HOIST_IN_A_LOOP_CONDITION,

                // the right of 'and' or of 'or', which is evaluated only when
                // the left did not already decide the answer. Lifted out, it
                // would be built even then
                HOIST_AFTER_AND,
                HOIST_AFTER_OR,
            };

            void walk(u32 node, u32 block, u32 statement, Hoisting hoisting);
            void walk_children(u32 node, u32 block, u32 statement,
                               Hoisting hoisting);

            // the first child under one rule and the rest under another,
            // which is the shape of 'while', 'and' and 'or' alike
            void walk_head_apart(u32 node, u32 block, u32 statement,
                                 Hoisting head, Hoisting rest);

            void lower_template_string(u32 node, u32 block, u32 statement);

            // what a refused one becomes, so that one mistake reads as one
            // error. See the comment on the definition
            void recover(u32 node, u32 block, u32 statement);

            // 'let __tsN : String'
            u32 make_declaration(u32 name_token, u32 like);

            // '__tsN.append(<what this chunk or interpolation holds>)'
            u32 make_append(u32 name_token, u32 piece);

            // the chunk as a string literal, or what the interpolation holds
            u32 argument_of(u32 piece);

            // the raw text of a chunk, wrapped in double quotes so that it is
            // a string literal. A template written with ' may hold a bare "
            // and that one has to be escaped; an escape already in the chunk
            // is passed through, because it meant something where it was
            // written and means the same thing here
            std::string quoted(const std::string& text);

            void insert_before(u32 block, u32 statement,
                               const std::vector<u32>& statements);

            void refuse(u32 node, Hoisting hoisting);

            AstNodeKind kind_of(u32 node);
            u32 first_child(u32 node);
            u32 sibling_of(u32 node);
            u32 token_of(u32 node);

        private:
            Module* module;
            AstBuilder builder;

            // per module, so two functions never argue about a name
            u32 counter;
    };
}

#endif
