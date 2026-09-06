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
            // the name of a type as a diagnostic should print it, written
            // back the way the source writes it. Public because the collector
            // and the override checker both report mismatches of their own
            std::string name_of(u32 type);

            // the same, reading a given module's table rather than the
            // current one: an instantiation's arguments live where its clone
            // does, which is the module that declared the generic
            std::string name_in(u32 owner, u32 type);

        private:
            // what an expression is, by kind. type_of is the funnel around it
            // and the only place record 0019's answer is written down, so this
            // one hands the answer back and records nothing itself
            u32 work(u32 scope, u32 node, u32 expected);

            // the node that carries the name a call names: the identifier
            // itself, the right side of a '.' or a '->', the last part of a
            // qualified form. It is where a diagnostic about the call points
            // and where the chosen declaration is recorded
            u32 name_of_callee(u32 node);

            // the name a named or generic type points at
            std::string declaration_name(u32 module, u32 candidate);

            // 'a[i]'. The element of an array, of a list, of a pointer, and
            // the VALUE of a hash -- whose subscript is asked to be its key
            u32 subscript(u32 scope, u32 node);

            // '&x' adds a pointer and '*p' takes one away
            u32 address_of(u32 scope, u32 node);
            u32 dereference(u32 scope, u32 node);

            // 'x as T' is whatever it was written as. Nothing checks that the
            // conversion makes sense: record 0018 has no implicit conversion
            // and says nothing about the explicit one, which is its own
            // question and not this phase's
            u32 cast(u32 scope, u32 node);

            // 'new T' and 'new T(a, b)' give back a T*. The arguments are
            // typed and nothing yet connects them to an 'init'
            u32 allocation(u32 scope, u32 node);

            // 'null' has no type of its own and takes the pointer its context
            // asks for -- record 0018's first rule, applied to the one other
            // thing in the language that is written without a type. With
            // nothing expected there is no answer to give, and that is said
            u32 null_literal(u32 node, u32 expected);

            // the 'init' candidates of one class, its own and not a base's:
            // record 0026 runs a base's before the derived's, so what is
            // written at a construction answers to that class alone
            std::vector<Candidacy> constructors_of(u32 type, u32& owner);

            // the arguments of a 'new T(...)' against those candidates, which
            // is a call in every way that matters
            void initialisation(u32 scope, u32 node, u32 made, u32 list);

            // '[a, b]', '{a, b}' and '(a, b)'. The first two hold one type,
            // which the context gives or the first element decides, and every
            // element after has to be it -- record 0018 has nothing that would
            // make two of them one. A tuple holds each element's own type
            u32 sequence(u32 scope, u32 node, u32 expected, bool array);
            u32 tuple(u32 scope, u32 node, u32 expected);

            // the element a container type holds, and INVALID_TYPE for a type
            // that holds none
            u32 element_of(u32 type);

            u32 literal(u32 node, u32 expected, BuiltinType fallback);
            u32 identifier(u32 scope, u32 node);
            u32 binary(u32 scope, u32 node, u32 expected, bool comparison);

            // 'and', 'or', 'not' and the symbol forms of all three. Record
            // 0018 has no conversion, and so it has no truthiness either: an
            // operand of these is a bool or it is a mistake, and there is
            // nothing an i32 could be turned into to make 'if n and m:' mean
            // something. 'unary' is 'not', which has one operand and no second
            // child to ask about
            // '&', '|', '^' and the shifts: arithmetic's shape with one
            // more rule, that a float has no bits to speak of
            u32 bitwise(u32 scope, u32 node, u32 expected);

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

            // the same, by a name that is not written anywhere: what record
            // 0034's operators are looked up by
            std::vector<Candidacy> members_named(u32 left,
                                                 const std::string& wanted,
                                                 u32& owner);

            // Record 0034. Whether the class on the left overloads this
            // operator, and the type of applying it if it does.
            //
            // INVALID_TYPE and nothing reported when the class declares no
            // such method: the operator's own rules then say what is wrong,
            // and 'cannot apply this to Array<i32> and i32' is a better
            // sentence than one about a method the reader never wrote.
            //
            // 'right' is 0 for a unary shape. The chosen method is written
            // down on the OPERATOR node, which is where the emitter looks:
            // nothing can work it out again, exactly as for a call
            u32 overloaded(u32 scope, u32 node, u32 left, u32 right);

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
            TypeBuilder builder;

            // record 0031's question, asked at the fourth of the four places
            // a value is given to something
            Coercion coercion;

            Module* module;
            u32 index;
    };
}

#endif
