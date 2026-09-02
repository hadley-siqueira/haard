#ifndef HAARD_OVERRIDE_CHECKER_H
#define HAARD_OVERRIDE_CHECKER_H

#include <haard/type_table/expression_typer.h>

namespace haard {
    // Checks what a method overrides, at the **declaration**.
    //
    // Record 0020 makes a method with the same parameters as one of its bases
    // an override of it, and keeps the return type out of that decision --
    // because record 0012 keeps the return out of what makes two overloads
    // different, and an override is that same comparison. Consistent, and it
    // left a hole: a derived class could give back anything at all and nothing
    // said a word.
    //
    // The rule this applies is C++'s, which is the only one that keeps a call
    // through the base honest: the return must be **the same type, or a
    // pointer or reference to something derived from what the base gives
    // back**. Somebody holding a Shape* and calling clone() has to get back
    // something that really is a Shape*, and a Square* is one. An f64 where an
    // i32 was promised is not.
    //
    // Covariance is only for a pointer and for a reference, never for a value,
    // and the reason is size: a caller through the base reserved room for the
    // base, and the derived one does not fit. C++ rejects the same thing for
    // the same reason.
    //
    // It runs over the symbol table and not the tree, like the TypeCollector,
    // and it needs every module's declarations already typed -- a base may
    // live in a module this one reached later. So it is a walk of its own,
    // after the types are in, which is the same rule every phase here has run
    // into: a phase that reads across modules cannot run inside the walk that
    // fills them.
    class OverrideChecker {
        public:
            OverrideChecker();

        public:
            void set_compilation(Compilation* compilation);

            void check(u32 module);

        private:
            // every method of this class, against the classes above it
            void check_class(u32 candidate);

            // the method of a base that this one overrides, as a candidacy
            // with module 0 when it overrides nothing. Same name, same
            // parameters -- record 0020's rule, and the first one found going
            // up, because a class between the two would have overridden it
            // first
            Candidacy overridden_by(u32 candidate, u32 super);

            // whether a method may give this back where the base gives that.
            //
            // Neither can be INVALID_TYPE: record 0016 poisons a whole
            // signature when any part of it will not build, and both callers
            // skip a candidate whose type is poisoned -- so a guard against it
            // here would be code against a state the phase before excludes
            bool may_give_back(u32 derived, u32 base);

            // a candidate's parameters, without the return, and its return on
            // its own. Both translated into the checked module's table, which
            // is what lets a method be compared with one from another module
            std::vector<u32> parameters_of(u32 module, u32 candidate);
            u32 result_of(u32 module, u32 candidate);

            // how many steps from one class up to another, -1 when it is not
            // above it at all
            int distance(u32 from, u32 to);

            // the identifier a declaration was named with, which is where a
            // diagnostic about the declaration points.
            //
            // NOT the written return type, tempting as that is: a composite
            // type node carries no token of its own -- AstBuilder::make_named_type
            // builds one with token 0 and lets the name below it hold the
            // span -- so reporting at a return type lands on the first token
            // of the file. The message names both types anyway; what the span
            // has to say is *which method*
            u32 name_node_of(u32 declaration);

            void report(u32 node, const std::string& message);

            std::string name_of(u32 type);
            std::string qualified(u32 module, u32 candidate);

        private:
            Compilation* compilation;
            TypeBuilder builder;
            ExpressionTyper typer;
            AstQuery query;

            Module* module;
            u32 index;
    };
}

#endif
