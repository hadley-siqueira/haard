#ifndef HAARD_IMPLICIT_COLLECTOR_H
#define HAARD_IMPLICIT_COLLECTOR_H

#include <haard/name_resolver/name_resolver.h>
#include <map>

namespace haard {
    // 'let' is not required: an assignment to a bare name that nothing in view
    // declares declares it, the way 'let n = 1' would.
    //
    // This used to ride along inside the SymbolCollector, and that was the one
    // place that cannot answer the question it asks. Deciding that 'n = 1'
    // declares n means knowing that n is not already something -- and
    // 'something' includes a field of a **base**. A base is a type, filled by
    // the pass after symbols are collected, so the collector's own lookup
    // climbed the scope chain, found nothing, and declared a local that stood
    // in front of the inherited field for the rest of the program. Silently:
    // the field was never written and the local was never read.
    //
    // So it runs here instead, in the type phase, where the NameResolver can
    // answer -- and that is the same rule every phase in this compiler has run
    // into, one level down: a question that reads across declarations cannot
    // be answered inside the walk that fills them.
    //
    // What it asks is this module's own view and not one import: the scope
    // chain outward, and the bases of every class along it. A local standing
    // in front of an imported name is what a 'let' does too, so that stays
    // deliberate and stays a per-module question.
    class ImplicitCollector {
        public:
            ImplicitCollector();

        public:
            void set_compilation(Compilation* compilation);

            // Whether it walked anything, for the same reason the
            // TypeCollector says so: record 0002's instantiation appends a
            // cloned declaration with a body of its own, and the body of a
            // clone declares names like any other. So the caller goes round
            // again until a round has nothing left to walk
            bool declare(u32 module);

        private:
            void walk(u32 node, u32 scope);

            // Only a plain '=' declares. 'n += 1' reads n before it writes it,
            // so a name it cannot find is a mistake and not a declaration, and
            // so is anything on the left that is not one identifier: 'a.b',
            // 'p->x' and 'a[i]' all name something that has to exist already
            void declare_target(u32 scope, u32 assignment);

        private:
            Compilation* compilation;
            NameResolver names;
            AstQuery query;

            Module* module;
            u32 index;

            // the node that opened a scope, back to the scope it opened. The
            // same reading of Scope::owner that every later phase does, and
            // for the same reason: one description of the scope shape
            std::map<u32, u32> scope_of;

            // how many of a module's declarations have been walked. A
            // declaration is a child of the root and never deeper, and a
            // clone is appended and never inserted, so a count is a watermark
            std::map<u32, u32> walked;
    };
}

#endif
