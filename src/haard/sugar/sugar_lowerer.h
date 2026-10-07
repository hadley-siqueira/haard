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
    // they were written in is not always meaning preserving. Where it is not
    // -- the right of 'and' and 'or', a loop's condition and step, an 'elif''s
    // condition -- the statement is rewritten first into the branch or the
    // loop it already is, where it is (record 0061). Only module level, with
    // no statement at all, is refused.
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
            // error per one written at module level
            void lower();

        private:
            void walk(u32 node, u32 block, u32 statement);
            void walk_children(u32 node, u32 block, u32 statement);

            // Record 0061: whether anything under this node would be built
            // before its statement -- a template string or a literal that is
            // not bound. Not through a block, which is a closure's body and
            // has statements of its own
            bool hoists(u32 node);

            // The four places where building before the statement would
            // change what the program does, each rewritten into a shape where
            // it does not. See the comment on 'walk'
            void short_circuit_into_a_branch(u32 node, u32 block,
                                             u32 statement);
            void condition_into_the_body(u32 node);
            void take_the_step_apart(u32 node, u32 block, u32 statement);
            void elif_into_an_else(u32 node);

            // 'let <name> : bool = <value>', '<name> = <value>' and
            // 'if not <condition>: break', which is what the four are made of
            u32 make_flag(u32 name, u32 value, u32 like);
            u32 make_assignment(u32 name, u32 value, u32 like);
            u32 make_exit_unless(u32 condition, u32 like);

            // statements put first in a block
            void prepend(u32 block, const std::vector<u32>& statements);

            // 'T[]' -> 'Array<T>', '[T]' -> 'List<T>' and '{K: V}' ->
            // 'Hash<K, V>', records 0016 and 0022. One rewrite for the three
            // of them: the node becomes a named type and its children become
            // the type arguments. A type and not an expression, so it hoists
            // nothing and can be written anywhere a type can -- including
            // where there is no statement at all
            void lower_into_generic(u32 node, const std::string& name);

            void lower_template_string(u32 node, u32 block, u32 statement);

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

            // 'let __arN = [1, 2, 3]' before the statement, and the literal
            // becomes a use of it. Record 0036's successor: the emitter
            // builds one where it is BOUND, because a fixed array is a C++
            // declaration and a declaration needs a statement
            void hoist_literal(u32 node, u32 block, u32 statement);

            // module level, where there is no statement to build before
            void refuse(u32 node, const std::string& what);

            // Record 0067: 'let (a, (b, _)) = t' is a 'let' per name, each
            // given its element -- 't[0]', 't[1][0]' -- and so each a copy,
            // record 0062, unless the type written for it says 'T&'. What is
            // not a name is bound to one first, so it is evaluated once. The
            // statement is replaced in its block by what it wrote, and those
            // are walked like any other
            bool is_destructuring(u32 statement);

            // and '(a, b) = (b, a)': the right side whole into a name, then
            // one assignment per place, so a swap swaps. A name that is not
            // in view is declared by its assignment, record 0027, which runs
            // after this pass and sees only 'a = __d0[0]'
            bool is_tuple_assignment(u32 statement);

            // and 'for (k, (v, _)) in c': the loop walks a name of its own,
            // and the body starts with a 'let' per name out of it -- each
            // marked to bind a REFERENCE, which is what a loop variable is
            // (record 0040) and what Hadley asked these to be
            void lower_pattern_loop(u32 for_each);

            // Record 0067: a 'switch' whose cases are tuples. It is a chain
            // of 'if's on a flag, so a case that does not match -- one whose
            // variant test fails deep inside -- leaves the next one to try,
            // and one that matches stops the rest:
            //
            //     let __m0 = false
            //     if not __m0 and t[0] == 0:      a literal or a value
            //         __m0 = true
            //         let x = t[1]                a name captures, by reference
            //         <the case's block>
            //     if not __m0:                    'default'
            //         ...
            //
            // An element written as a call, 'Some(y)', is a variant and is
            // matched by a 'switch' of its own around the rest. A bare name is
            // always a capture, so a variant with nothing to carry is written
            // with its enum, 'Option.None'. Nothing matching and no 'default'
            // runs nothing (Hadley, 2026-10-07)
            bool is_tuple_switch(u32 statement);
            void lower_tuple_switch(u32 node, u32 block);
            void take_pattern_apart(u32 pattern, u32 held,
                                    const std::vector<u32>& path,
                                    std::vector<u32>& tests,
                                    std::vector<u32>& captures,
                                    std::vector<std::vector<u32>>& at,
                                    std::vector<u32>& variants,
                                    std::vector<std::vector<u32>>& where);
            u32 joined(AstNodeKind kind, TokenKind token, const char* text,
                       const std::vector<u32>& parts, u32 like);
            void assign_apart(u32 statement, u32 block);
            void assign_into(u32 pattern, u32 source,
                             const std::vector<u32>& path,
                             std::vector<u32>& written);
            void destructure(u32 statement, u32 block);
            // 'through' when the type written for the whole was a reference
            // to a tuple, '(A, B)&': then each name refers to its element
            void destructure_into(u32 pattern, u32 type, u32 source,
                                  const std::vector<u32>& path, bool constant,
                                  std::vector<u32>& written,
                                  bool through = false);

            // and a parameter that is a pattern, '@(x, y) : (f64, f64)' or
            // '|(x, y)| {...}': it becomes a name of its own and the body
            // starts with a 'let' per name out of it. With no type written --
            // a closure typed by where it goes -- each name refers into the
            // parameter, as in a 'for'
            void lower_parameter_patterns(u32 function);
            void take_parameter_apart(u32 name, u32 type,
                                      std::vector<u32>& written);
            u32 element_at(u32 source, const std::vector<u32>& path, u32 like);
            u32 declaration_of(u32 name, u32 type, u32 value, bool constant,
                               u32 like);

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
