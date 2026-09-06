#ifndef HAARD_STATEMENT_CHECKER_H
#define HAARD_STATEMENT_CHECKER_H

#include <haard/type_table/expression_typer.h>
#include <map>

namespace haard {
    // Checks the statements of a module against the types around them, which
    // until now nothing did: every expression kind typed, but the typer was
    // only ever *called* on a binding's initialiser, so 'def f : i32' with
    // 'return 2.5' passed in silence and so did 'if 5:'.
    //
    // It needed no new decision. Record 0018 has no implicit conversion, so
    // every question here is equality, and the one thing that is not a
    // comparison is a literal -- which takes the type the statement asks for
    // rather than being converted into it. That is why every check hands its
    // wanted type down to the typer as 'expected' instead of typing the
    // expression first and comparing afterwards: 'return 1' in a 'u8'
    // function is a u8 literal, and '300' there is an error about the value.
    //
    // Three statements ask a question:
    //
    //   'return' against the type its function returns, which record 0016 put
    //   last in the signature the collector built
    //
    //   the condition of an 'if', an 'elif', a 'while' and the middle part of
    //   a C shaped 'for', against bool
    //
    //   the two sides of an assignment, the left one deciding and the right
    //   one asked to be it
    //
    // The walk is the UseResolver's: recurse over everything and read
    // Scope::owner backwards to know which scope a node sits in, so there is
    // one description of the scope shape and this is not a second copy of it.
    // Both the scope and the function's return type travel as parameters and
    // not as members, which is what makes a function inside a function right
    // without anything having to be saved and put back.
    //
    // What it must not do is type an expression twice, because the typer
    // reports as it goes and a second call would report again. A binding's
    // initialiser belongs to the TypeCollector's second pass and is walked
    // through here without being typed.
    //
    // An assignment is checked wherever it is written and not only where a
    // block holds it, which is safe for the same reason: the typer has no
    // type for an assignment and stops at one instead of descending into it.
    // So nothing else ever types the two sides, here or in a condition or in
    // an argument, and 'a = b = 1' has both of its written exactly once.
    class StatementChecker {
        public:
            StatementChecker();

        public:
            void set_compilation(Compilation* compilation);

            // checks every statement of this module, logging one error per
            // question that came back wrong
            void check(u32 module);

        private:
            // 'result' is the type the function being walked returns, and
            // INVALID_TYPE outside any function or inside one whose signature
            // could not be built
            void walk(u32 node, u32 scope, u32 result);

            void check_return(u32 node, u32 scope, u32 result);

            // an expression written on a line of its own, for what it does
            // and not for what it is: a call whose answer is thrown away.
            // Nothing typed those until 2026-09-02, and they are most of the
            // calls a real program makes -- 'println(x)', 'list.add(x)'.
            //
            // Being a statement is the whole of what makes one reachable from
            // here, so this is the one check that reads the block and not the
            // node
            void check_expression(u32 node, u32 scope);
            void check_condition(u32 node, u32 scope);
            void check_assignment(u32 node, u32 scope);

            // the type a 'def' gives back: the last argument of the signature
            // on its candidate. INVALID_TYPE when there is no signature to
            // read, which is a declaration whose type already failed to build
            // and was already reported where it was written
            u32 result_of(u32 function);

            // Whether this declaration still has parameters nothing has
            // bound. Record 0002: a use of a generic names the **clone**, and
            // the clone is what gets checked -- the uninstantiated body is not
            // a program yet and nothing in it has a type to check against
            bool is_an_unbound_generic(u32 node);

            void report(u32 node, const std::string& message);

            AstNodeKind kind_of(u32 node);
            u32 first_child(u32 node);
            u32 second_child(u32 node);

        private:
            Compilation* compilation;
            ExpressionTyper typer;
            Coercion coercion;

            Module* module;
            u32 index;

            // the node that opened a scope, back to the scope it opened
            std::map<u32, u32> scope_of;
    };
}

#endif
