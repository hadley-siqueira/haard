#ifndef HAARD_FOR_EACH_LOWERER_H
#define HAARD_FOR_EACH_LOWERER_H

#include <haard/ast/ast_builder.h>
#include <haard/symbol_table/symbol_collector.h>
#include <haard/type_table/expression_typer.h>
#include <set>
#include <utility>

namespace haard {
    class TypeCollector;

    // Record 0040. 'for x in <something>' is written form for one of three
    // loops, and which one it is depends on **what is being walked**:
    //
    //     for x in xs:            a class, walked by the cursor it gives back
    //     for x in fixed:         a fixed array, walked by its own length
    //     for i in 0..10:         a range, which is a plain loop and no class
    //
    // So this is the second Ast -> Ast pass, and the one that cannot be the
    // first. SugarLowerer runs before the symbols are collected, which is
    // where sugar belongs; this one has to know what the sequence **is**, and
    // a 'let xs = [1, 2, 3]' is an 'Array<i32>' only after inference. It runs
    // inside the type phase for that reason, driven by the loop variable's
    // own candidate: the collector reaches it in the order the source wrote
    // it, so 'xs' is typed before the loop and a 'let y = x' below it is
    // typed after.
    //
    // What comes out is an ordinary tree. Nothing downstream knows a foreach
    // existed:
    //
    //     for x in xs:                    {
    //         total = total + x               let __c0 = xs.iterator()
    //                                         while __c0.has_next():
    //                                             let x = __c0.next()
    //                                             total = total + x
    //                                     }
    //
    // The braces are the point of the block: it is where the cursor lives, so
    // two loops in one function are two cursors and two 'x'es and neither
    // sees the other. The loop node is rewritten **in place** into that
    // block, which is what keeps every other link in the tree correct -- and
    // what keeps the scope the collector opened for the loop, since a scope
    // is stamped with the node that opened it.
    //
    // 'x' is a **reference** and needs nothing built to be one: a cursor's
    // 'next' gives back a 'T&' and a binding keeps it, so writing 'x' writes
    // the element. Record 0031 is why it matters -- a copy of an element that
    // owns something is refused, and a foreach that copied would not compile
    // over an 'Array<String>' at all.
    //
    // The index form was free and was **rejected**: 'List's operator[] walks
    // from the head, so a foreach written with one would turn an O(n) method
    // into an O(n^2) loop, silently, in the one construct that exists to be
    // the obvious way to walk a thing. Do not re-derive it.
    class ForEachLowerer {
        public:
            ForEachLowerer();

        public:
            void set_compilation(Compilation* compilation);

            // the typer this pass builds with is one of the two that can
            // instantiate a generic, and record 0039 is what makes that safe
            // here: a cursor asked for on an array built from a literal is a
            // clone made mid-sweep, and it is typed on the spot
            void set_collector(TypeCollector* collector);

            // Takes the loop apart, and gives back the AST_BINDING the loop
            // variable now points at -- 0 when the loop was left standing,
            // which is every refusal and is always something reported.
            //
            // 'written' comes back as the type the variable must be given
            // when this pass is the one that knows it, which is the range:
            // 'for i in 0..n' with an n of u32 makes i a u32, and inference
            // reading '0' alone would make it an i32. INVALID_TYPE
            // everywhere else, where what the binding was given decides
            u32 lower(u32 index, u32 scope, u32 for_each, u32& written);

        private:
            // the three shapes, each giving back the loop variable's binding
            u32 over_a_cursor(u32 scope, u32 for_each, u32 variable,
                              u32 container);
            u32 over_a_fixed_array(u32 scope, u32 for_each, u32 variable,
                                   u32 container, u32 length);
            u32 over_a_range(u32 scope, u32 for_each, u32 variable, u32 range,
                             u32& written);

            // 'for ; <condition> ; <name> = <name> + 1:' around a body, which
            // is what the fixed array and the range both walk with. A C
            // shaped 'for' and not a 'while' with the step at the end of the
            // body, because a 'continue' would jump over that step
            u32 counted_loop(u32 name, u32 condition, u32 body);

            // '<name> < <limit>', or '<=' for an inclusive range
            u32 condition_of(u32 name, u32 limit, bool inclusive);

            // 'let __xN = <expression>', declared in this scope and given
            // this type. The type is written down rather than left to the
            // next round of inference because the loop variable is typed
            // **now**, out of an expression that names this local
            u32 make_local(u32 scope, u32 name, u32 expression, u32 type);

            // 'let <the name the loop was written with> = <expression>'. The
            // identifier the source wrote is reused as the binding's name, so
            // the diagnostics, the text and the position are all still the
            // author's
            u32 make_loop_binding(u32 variable, u32 expression);

            // '<receiver>.<method>()', the calls a cursor answers
            u32 call_on(u32 receiver, const std::string& method);

            u32 identifier_like(u32 token);
            u32 integer(const std::string& digits);
            u32 name_for(const std::string& prefix);

            // the loop becomes this block, holding these statements
            void rewrite_into_block(u32 for_each,
                                    const std::vector<u32>& statements);
            void prepend(u32 block, u32 statement);

            // What is inside a generic nobody instantiated is not a program
            // yet (record 0002), and its sequence has no type worth asking
            // about: 'T' is a type parameter. The clone is where this runs,
            // with every parameter bound -- which is the same rule the
            // statement checker and the emitter already follow
            bool inside_an_unbound_generic(u32 scope);

            void report(u32 node, const std::string& message);

            AstNodeKind kind_of(u32 node);
            u32 child_of(u32 node, AstNodeKind kind);
            u32 first_child(u32 node);
            u32 sibling_of(u32 node);
            u32 token_of(u32 node);

        private:
            Compilation* compilation;
            Module* module;
            u32 index;

            AstBuilder builder;
            AstQuery query;
            SymbolCollector symbols;
            ExpressionTyper typer;

            // the token every synthetic one is made to look like, so that a
            // diagnostic about what this pass wrote points at the 'for' the
            // author wrote
            u32 like;

            // One attempt per loop, whatever it ends in. 'for key, value in
            // pairs' declares two names and so reaches this twice, and a
            // refusal reported twice is one mistake read as two. By (module,
            // node), because a node index means nothing outside its own tree
            std::set<std::pair<u32, u32>> seen;

            // across the whole compilation, so nothing has to argue about
            // whose turn it is to reset it
            u32 counter;
    };
}

#endif
