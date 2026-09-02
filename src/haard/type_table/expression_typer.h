#ifndef HAARD_EXPRESSION_TYPER_H
#define HAARD_EXPRESSION_TYPER_H

#include <haard/name_resolver/overload_resolver.h>

namespace haard {
    // What type an expression has, per record 0018.
    //
    // A literal has **no type of its own**: it takes the one the context asks
    // for, which is why every entry point here carries an 'expected'. That is
    // the machinery record 0018 leans on to have no implicit conversion at all
    // and still let 'let n: i64 = 1' and 'f(3)' be written. With nothing
    // expected an integer literal is an i32 and a float literal an f64.
    //
    // A literal that does not fit is an error about the **value** and not about
    // a type, which is the shape record 0018 asked for: '300 does not fit in
    // u8', not 'cannot convert i32 to u8'.
    //
    // Everywhere else the rule is equality and nothing more. Two operands of an
    // operator have to be the same type, because record 0018 has no conversion
    // to make them one, and the diagnostic says so at the operator.
    //
    // What it cannot type yet, and gives back INVALID_TYPE for without
    // complaining: a string or symbol literal, which has no type until the
    // prelude declares one (record 0017 leaves what else it holds open).
    class ExpressionTyper {
        public:
            ExpressionTyper();

        public:
            void set_compilation(Compilation* compilation);

            // 'expected' is INVALID_TYPE when the context asks for nothing,
            // which is what a 'let' with no written type does
            u32 type_of(u32 module, u32 scope, u32 node, u32 expected);

            // which module name_of reads its names out of. type_of sets it
            // too, and this is for a caller that has to name a type before it
            // has asked for one -- the StatementChecker reporting a 'return'
            // with nothing after it
            void set_module(u32 module);

        public:
            // the name of a type as a diagnostic should print it. Public
            // because the collector reports a mismatch it found itself
            std::string name_of(u32 type);

        private:
            u32 literal(u32 node, u32 expected, BuiltinType fallback);
            u32 identifier(u32 scope, u32 node);
            u32 binary(u32 scope, u32 node, u32 expected, bool comparison);

            // 'and', 'or', 'not' and the symbol forms of all three. Record
            // 0018 has no conversion, and so it has no truthiness either: an
            // operand of these is a bool or it is a mistake, and there is
            // nothing an i32 could be turned into to make 'if n and m:' mean
            // something. 'unary' is 'not', which has one operand and no second
            // child to ask about
            u32 logical(u32 scope, u32 node, bool unary);

            // whether this operand is a bool, complaining when it is not. It
            // asks for a bool rather than typing first, so 'flag and 1' is a
            // literal that cannot be one and says so about the literal
            bool boolean_operand(u32 scope, u32 node, u32 at);

            // 'a.b' and 'a->b'. Record 0018: '.' reads a member of a T and
            // of a T* alike, looking through one level of pointer when there
            // is one, and '->' is the explicit form that only a pointer may be
            // written with.
            //
            // The right side is not a name in any scope --
            // it is a member of whatever the left side turned out to be, which
            // is why the UseResolver refuses it and this is where it belongs.
            //
            // The walk is the whole family joined up: the left side's type is
            // a TYPE_NAMED, which points at the candidate of a class, whose
            // scope holds the members, and an inherited one is the same search
            // one step up Candidate::super
            u32 member(u32 scope, u32 node, bool through_pointer);

            // 'f(a, b)'. The callee is a name and its candidates are what the
            // resolver chooses among; the arguments are typed first, with a
            // literal left untyped so each candidate may ask it to be its own
            // parameter -- which is record 0018's first rule meeting its
            // twelfth
            u32 call(u32 scope, u32 node);

            // the candidates a callee names: a bare name, one of the two
            // qualified forms, or a member of whatever is on the left of a
            // '.' or a '->'
            std::vector<Candidacy> callee_of(u32 scope, u32 node);

            // Everything a name means inside a class and its bases, as
            // candidates rather than as one type. A field access wants the
            // type and a method call wants the set, and they are the same walk
            // -- so this is the walk and 'member' reads the first answer's
            // type off it.
            //
            // 'owner' comes back as the module the class lives in
            std::vector<Candidacy> members_of(u32 left, u32 name, u32& owner);

            // the type 'this' has: a pointer to the class the method was
            // written in. A pointer and not a reference because that is what
            // a method receives, and the '.' of record 0018 reads a member of
            // either alike
            u32 this_type(u32 scope);

            // the candidate of the class a type names, looking through one
            // reference. Gives back 0 for anything that has no members
            u32 class_of(u32 type, u32& owner);

            // whether an integer literal's digits fit the builtin it is being
            // asked to be. The check record 0018 wanted: about the value
            bool fits(u32 node, u32 type);

            void report(u32 node, const std::string& message);

            std::string text_of(u32 node);
            AstNodeKind kind_of(u32 node);
            u32 first_child(u32 node);
            u32 second_child(u32 node);

        private:
            Compilation* compilation;
            NameResolver resolver;
            OverloadResolver overloads;

            Module* module;
            u32 index;
    };
}

#endif
