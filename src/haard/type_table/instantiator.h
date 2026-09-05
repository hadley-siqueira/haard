#ifndef HAARD_INSTANTIATOR_H
#define HAARD_INSTANTIATOR_H

#include <haard/ast_query/ast_query.h>
#include <haard/module/module.h>
#include <vector>

namespace haard {
    class Compilation;

    // Record 0002, made real: a generic written with arguments becomes a
    // declaration of its own, cloned from the one the source wrote and with
    // its type parameters bound to the arguments.
    //
    // Everything after this point is the point. The clone is an ordinary
    // class -- an ordinary tree under the module root, ordinary names in
    // ordinary scopes -- so overload resolution, the statement checker, the
    // override checker and the emitter read it without knowing a generic
    // exists. Nothing downstream carries a substitution map, and nothing
    // downstream had to change.
    //
    // Two rules make it work, and both are about where things live:
    //
    // 1. **The clone lives in the module that declared the generic**, never
    //    in the one that used it. A name inside the body then resolves in the
    //    scope it was written in, which is record 0002's consequence 2 and
    //    the hygiene bug C++'s two-phase lookup exists to avoid.
    //
    // 2. **The clone is declared under a name no source can write** --
    //    'Pair#3#7', the arguments by their type index. It has to be in a
    //    scope, because that is the only way to make a candidate; it must not
    //    be reachable by a lookup, or a bare 'Pair' would find an
    //    instantiation. An unwritable name is both.
    //
    // A parameter is bound by setting the **type** of its candidate. The one
    // place that reads it is TypeBuilder, which gives back the bound type
    // instead of a TYPE_GENERIC -- so inside the clone, 'A' simply *is* i32,
    // and no walk substitutes anything.
    class Instantiator {
        public:
            Instantiator();

        public:
            void set_compilation(Compilation* compilation);

            // The candidate of the class this instantiation is of, in the
            // module that declares it, or 0 when the arguments do not fit --
            // which is reported, at 'node' in the module that wrote it.
            //
            // 'arguments' belong to the DECLARING module's table: the caller
            // translates them on the way in, because the clone is made there
            // and a type index does not cross a module boundary
            u32 instantiate(u32 caller, u32 node, u32 owner, u32 candidate,
                            const std::vector<u32>& arguments);

        private:
            // 'Pair<Pair<Pair<...>>>' is a program that never stops
            // instantiating, and nothing about it is a mistake the source can
            // see. So the nesting is capped and the cap is a diagnostic
            u32 depth_of(u32 owner, u32 type);

            void report(u32 caller, u32 node, const std::string& message);

        private:
            Compilation* compilation;
    };
}

#endif
