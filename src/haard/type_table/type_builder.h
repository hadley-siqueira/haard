#ifndef HAARD_TYPE_BUILDER_H
#define HAARD_TYPE_BUILDER_H

#include <haard/name_resolver/name_resolver.h>
#include <haard/type_table/instantiator.h>
#include <haard/type_table/type_table.h>

namespace haard {
    class TypeCollector;

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

            // Who to tell when a generic is instantiated, so the clone is
            // typed before anything asks it a question. Null outside the type
            // phase, where nothing is being collected to catch up with
            void set_collector(TypeCollector* collector);

            // the type this node names, seen from this scope of this module.
            // INVALID_TYPE when the tree says nothing a type can be built from
            u32 build(u32 module, u32 scope, u32 node);

            // A generic instantiation the SOURCE never wrote: 'Array<i32>'
            // for a '[1, 2, 3]'. Records 0017 and 0022 already have the
            // compiler knowing the **name** 'Array' and nothing else about
            // it, and this is that knowledge being used -- resolved in the
            // scope the literal was written in, so a program that declares
            // its own 'Array' gets its own, which is the same rule record
            // 0009 gives an importer over its imports.
            //
            // 'at' is the node a diagnostic about it points at
            u32 build_generic(u32 module, u32 scope, u32 at,
                              const std::string& name,
                              const std::vector<u32>& arguments);

            // The same type, written into another module's table. Record
            // 0016: a type crossing a module boundary is translated at the
            // import and not probed, because comparing two types across a
            // boundary is structural and recursive while comparing two names
            // is a hash and a memcmp.
            //
            // A builtin translates to itself for free -- their indices are the
            // same in every module, which is what that decision bought
            u32 translate(u32 into, u32 from, u32 type);

            // Record 0045. The type a **callee** names, for a construction:
            // 'String("abc")' is written as a call, so the name arrives as an
            // expression node -- an identifier, a scope, or the name and
            // arguments of a generic -- and never as an AST_NAMED_TYPE.
            //
            // Those are the same two parts an AST_NAMED_TYPE holds, in the
            // same order, so this is that node's own builder reached without
            // the node: nothing is synthesised into the tree, and a written
            // generic instantiates here exactly as it does in a type
            u32 build_written_name(u32 module, u32 scope, u32 name,
                                   u32 arguments);

            // Whether this candidate set names a **type** rather than a set
            // of overloads, and which declaration it is. Public for record
            // 0045: a call has to ask before it ranks, because a type answers
            // to no signature and would score -1 against every argument
            u32 type_symbol(const std::vector<Candidacy>& found, u32& owner);

        private:
            // the two above, with the members set. The public pair saves
            // and puts back what the caller was looking at, so that an
            // instantiation re-entering the type phase is transparent
            u32 build_here(u32 module, u32 scope, u32 node);
            u32 build_generic_here(u32 module, u32 scope, u32 at,
                                   const std::string& name,
                                   const std::vector<u32>& arguments);

            u32 build_named(u32 module, u32 scope, u32 node);

            // the body both of them share: the name and its arguments, with
            // the members already pointing at the module being read
            u32 build_written_name_here(u32 module, u32 scope, u32 name,
                                        u32 arguments);

            // 'T[n]' keeps n as a value. Only an integer literal can be read
            // today: a 'const N' length needs constant evaluation, which does
            // not exist
            u32 length_of(u32 node);

            u32 first_child(u32 node);
            u32 second_child(u32 node);
            AstNodeKind kind_of(u32 node);

        private:
            Compilation* compilation;
            TypeCollector* collector;
            NameResolver resolver;
            Instantiator instantiator;

            Module* module;
            u32 index;
    };
}

#endif
