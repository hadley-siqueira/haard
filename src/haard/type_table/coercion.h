#ifndef HAARD_COERCION_H
#define HAARD_COERCION_H

#include <haard/type_table/type_builder.h>

namespace haard {
    // Record 0018's closed list of coercions, in one place.
    //
    // It lived inside OverloadResolver::match until 2026-09-03, which made it
    // a rule about **arguments** rather than a rule of the language. Writing
    // ordinary Haard found what that costs: 'return d' from a function giving
    // back a 'Base&', 'let b : Base& = d' and 'b = d' all failed, because the
    // return, the binding and the assignment each compared by equality and
    // only a call knew about the list. A rule the language has in one place
    // and not in four is the reason this class exists.
    //
    // The list, per record 0018 and agenda 1.21:
    //
    //   a reference or a pointer to a derived type where a base is expected,
    //   costing one step per class climbed
    //
    //   a 'char*' where a 'String' is expected, costing one
    //
    // 'null' where a pointer is expected is not here: it is a literal, and a
    // literal takes the type its context asks for rather than being converted
    // into it. The same goes for one level of dereference under a '.', which
    // is about reaching a member and not about giving a value to somebody.
    //
    // What it does **not** do is rank literals. A literal has no type to be
    // coerced from, so an argument that is one never reaches here -- the
    // OverloadResolver asks it to *be* the parameter instead.
    class Coercion {
        public:
            Coercion();

        public:
            void set_compilation(Compilation* compilation);

            // how far 'given' is from 'wanted', and -1 when nothing on the
            // list gets there. 0 is the same type, and a bigger number is a
            // worse match -- which is the only ranking record 0018 has
            int steps(u32 module, u32 given, u32 wanted);

            // whether 'given' can be given to something asking for 'wanted'.
            // What the return, the binding and the assignment ask: they rank
            // nothing, so the distance is not their business
            bool fits(u32 module, u32 given, u32 wanted);

            // how many steps from one class up to another, or -1 when it is
            // not above it at all. Single inheritance makes this a walk.
            // Public because record 0018's ranking is not the only reader:
            // telling an override from an overload is the same walk, over the
            // classes that hold two methods
            int climb(u32 module, u32 from, u32 to);

            // Whether a value of this type may be given to something by
            // **copy**. Record 0031: a class that declares 'destroy' owns
            // something, and one that owns something and has not said how to
            // be copied cannot be -- the C++ that used to come out of that
            // copied the pointer and ran the destructor twice.
            //
            // True for everything that is not a class held by value: a
            // pointer and a reference name a thing rather than holding one,
            // and a builtin owns nothing.
            //
            // It lives here for the reason record 0018's list does: the four
            // places a value is given to something -- a call, a return, a
            // binding and an assignment -- must all ask, and the question has
            // to have one answer. Walks the bases, because what a base owns
            // is what a derived class holds
            bool may_be_copied(u32 module, u32 type);


        private:
            // whether the class this candidate names, or any class above it,
            // declares a member of this name
            bool declares(u32 module, u32 candidate, const std::string& name);

            bool is_char_pointer(u32 module, u32 type);

            // the standard library's String, which agenda 1.21 needs to name
            // and no record has given the compiler a way to find yet.
            //
            // By its **name**, and knowingly: record 0017 already has the
            // compiler knowing the name 'Array' and nothing else about it, so
            // this is the same kind of knowledge and not a new one. When the
            // prelude exists this becomes identity against the declaration it
            // holds, and a program that declares its own String stops
            // answering to a string literal -- which is the tightening record
            // 0017's lookup order already promises for 'Array'
            bool is_string(u32 module, u32 type);

        private:
            Compilation* compilation;
            TypeBuilder builder;
    };
}

#endif
