#ifndef HAARD_TYPE_BUILDER_H
#define HAARD_TYPE_BUILDER_H

#include <haard/name_resolver/name_resolver.h>
#include <haard/type_table/type_table.h>

namespace haard {
    // Turns a type as the parser wrote it into an index in the module's type
    // table. The fifth of the family, and the same division as everywhere
    // else: TypeTable holds and interns, this one reads a tree and asks for
    // entries.
    //
    // A named type is the only part that needs anything outside the tree: it
    // has to know which declaration the name means, so it goes through the
    // NameResolver and reaches its two qualified forms as well. It reports
    // nothing -- a name nothing declares is the UseResolver's diagnostic, and
    // this gives back 0 for it.
    //
    // The nesting is the parser's and this adds none of its own. Every postfix
    // applies in written order, so 'i32[10]*[5]' is an array of five pointers
    // to a ten element array, and the entries come out in that order because
    // Parser::parse_type_postfix built them that way.
    class TypeBuilder {
        public:
            TypeBuilder();

        public:
            void set_compilation(Compilation* compilation);

            // the type this node names, seen from this scope of this module.
            // INVALID_TYPE when the tree says nothing a type can be built from
            u32 build(u32 module, u32 scope, u32 node);

            // The same type, written into another module's table. Record
            // 0016: a type crossing a module boundary is translated at the
            // import and not probed, because comparing two types across a
            // boundary is structural and recursive while comparing two names
            // is a hash and a memcmp.
            //
            // A builtin translates to itself for free -- their indices are the
            // same in every module, which is what that decision bought
            u32 translate(u32 into, u32 from, u32 type);

        private:
            u32 build_named(u32 module, u32 scope, u32 node);

            // the declaration a type name means: a class, a struct, an
            // enum, a union or a generic parameter. Anything else of that
            // name is not a type, and a candidate set may hold both
            u32 type_symbol(const std::vector<Candidacy>& found, u32& owner);

            // 'T[n]' keeps n as a value. Only an integer literal can be read
            // today: a 'const N' length needs constant evaluation, which does
            // not exist
            u32 length_of(u32 node);

            u32 first_child(u32 node);
            u32 second_child(u32 node);
            AstNodeKind kind_of(u32 node);

        private:
            Compilation* compilation;
            NameResolver resolver;

            Module* module;
            u32 index;
    };
}

#endif
