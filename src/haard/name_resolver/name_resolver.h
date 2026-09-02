#ifndef HAARD_NAME_RESOLVER_H
#define HAARD_NAME_RESOLVER_H

#include <haard/compilation/compilation.h>
#include <string>
#include <vector>

namespace haard {
    // Where one candidate lives. Record 0012 makes a reference across a module
    // boundary the pair (module, symbol) and never a name, and record 0013
    // made that pair transient: it is what a lookup gives back, and never a
    // field of anything written to disk
    struct Candidacy {
        u32 module;
        u32 candidate;
    };

    // The first half of name resolution: every declaration a name may mean,
    // gathered. Choosing among them by signature is the second half, and it
    // waits on the type table (2.6) and on the logic of implicit conversion
    // (1.13), neither of which exists.
    //
    // It gathers and it never decides. Two candidates that a call would find
    // equally good are an ambiguous *call*, which record 0012 puts at the call
    // and not here, and two imports bringing one name are record 0009's
    // warning at the use -- so both come back in this list, in the order the
    // records read them, and the phase that reports them is not this one.
    //
    // The walk, per records 0009, 0008 and 0013:
    //
    //   the scope chain of the module itself, outward through Scope::parent,
    //   then the module scope of each import, in the order the source wrote
    //   the imports
    //
    // Nothing is merged at an import (record 0013), so the second half of that
    // walk asks each dependency's own tables. The importer's interned index
    // means nothing there, which is why the name crosses as a hash and its
    // bytes, and record 0014's hash is what keeps that a probe instead of a
    // string comparison per dependency.
    class NameResolver {
        public:
            NameResolver();

        public:
            void set_compilation(Compilation* compilation);

            // every declaration this name may mean, seen from this scope of
            // this module. Empty when nothing declares it
            std::vector<Candidacy> resolve(u32 module, u32 scope,
                                           const std::string& name);

            // '::name'. It needs no logic of its own: record 0009 makes the
            // leading '::' skip the local scope and the enclosing class, which
            // is the walk above started from the module scope instead of from
            // where the name was written. The grammar had this the moment the
            // walk took a starting scope
            std::vector<Candidacy> resolve_at_module(u32 module,
                                                     const std::string& name);

            // 'alias::name'. Only the imports written under that alias, and
            // they are the same imports the flat form already reaches: an
            // alias adds a way to name something and never withholds it
            // (records 0008 and 0009). A star's alias covers its whole
            // expansion (record 0006), so this can be as ambiguous as the bare
            // form and resolves the same way, by the sorted order
            std::vector<Candidacy> resolve_qualified(u32 module,
                                                     const std::string& alias,
                                                     const std::string& name);

        private:
            // the candidates of one symbol, appended in declaration order
            void gather(std::vector<Candidacy>& found, u32 module, u32 symbol);

            // and what the classes ABOVE this one declare of the same name.
            //
            // Hadley, 2026-09-02: a derived class sees what its bases declare,
            // by a bare name and not only through a '.'. The scope chain
            // cannot reach that on its own -- it is lexical, and a base is not
            // lexically around anything -- so this is the one step it has to
            // be told to take.
            //
            // Crossing into the base's module means interning the name there
            // first, which is record 0013's rule for a lookup that crosses an
            // import. Gives back whether something that is not a function was
            // gathered, which shadows and stops the walk outward
            bool gather_bases(std::vector<Candidacy>& found, u32 module,
                              u32 declaration, u32 hash,
                              const std::string& name);

            // whether this node declares a type, which is the only kind of
            // scope that has bases to look in
            bool declares_a_type(u32 module, u32 node);

            // whether a symbol's candidates are all functions. A name that is
            // anything else shadows: Hadley, 2026-09-02, settling what record
            // 0012 left for the class boundary -- a non function stops the
            // walk outward, a function does not, so a field hides a global of
            // its name while a method joins the overload set of one
            bool only_functions(u32 module, u32 symbol);

            // this module's index for a name hashed elsewhere, and the symbol
            // it names in that module's scope. 0 when the module has no such
            // name, which the string table answers before any scope is walked
            u32 module_symbol(u32 module, u32 hash, const std::string& name);

        private:
            Compilation* compilation;
    };
}

#endif
