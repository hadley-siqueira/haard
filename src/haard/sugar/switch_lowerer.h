#ifndef HAARD_SWITCH_LOWERER_H
#define HAARD_SWITCH_LOWERER_H

#include <haard/ast/ast_builder.h>
#include <set>
#include <utility>
#include <haard/symbol_table/symbol_collector.h>
#include <haard/type_table/expression_typer.h>

namespace haard {
    class TypeCollector;

    // Record 0043. A 'switch' over something C++ cannot switch over becomes a
    // chain of 'if's, which Hadley asked for in those words: *pode traduzir
    // para if else se for mais fácil*.
    //
    // An enum, an integer and a char stay a C++ switch -- a jump table is what
    // that statement is for. Everything else is compared, and comparing is
    // what 'if' does:
    //
    //     switch name:                {
    //         case "ready":               let __sw0 = name
    //         case "done":                if __sw0 == "ready" or __sw0 == "done":
    //             f()                         f()
    //         default:                    else:
    //             g()                         g()
    //                                 }
    //
    // The subject is bound to a name **once**, because a chain reads it once
    // per case and 'switch f():' would otherwise call f as many times as there
    // are patterns.
    //
    // It runs in the type phase, after inference, for the same reason record
    // 0040's lowering does: it has to know what it is walking. What it writes
    // is an ordinary tree, so the statement checker types the comparisons the
    // way it types every other condition -- and a subject whose class has no
    // 'operator==' is reported by that, in words about the operator the reader
    // would have to write.
    class SwitchLowerer {
        public:
            SwitchLowerer();

        public:
            void set_compilation(Compilation* compilation);
            void set_collector(TypeCollector* collector);

            // takes apart every switch of this module that cannot be one in
            // C++, and says whether it took any -- the caller types what was
            // written afterwards
            bool lower(u32 index);

        private:
            void walk(u32 node, u32 scope);

            // whether this subject is one C++ can switch over: an enum, an
            // integer or a char. A bool is not one, and neither is a float,
            // and neither is any class
            bool switchable(u32 given);

            void rewrite(u32 node, u32 scope, u32 given);

            // '__sw0 == <pattern>', joined by 'or' when several cases share a
            // block
            u32 condition_of(u32 name, u32 one_case, u32 from, u32 to);

            u32 make_local(u32 scope, u32 name, u32 subject, u32 type);

            // whether an expression has no name to refer to, which is
            // what decides between binding the subject and copying it
            bool is_an_rvalue(u32 node);

            AstNodeKind kind_of(u32 node);
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

            // the token every synthetic one is made to look like, so a
            // diagnostic about what this pass wrote points at the 'switch'
            u32 like;

            // One look per switch, whatever it ends in. The pass runs on
            // every round of the type phase's loop, and a subject that does
            // not type reports through the typer -- once is once
            std::set<std::pair<u32, u32>> seen;

            // across the compilation, so two modules never argue about a name
            u32 counter;
            bool worked;
    };
}

#endif
