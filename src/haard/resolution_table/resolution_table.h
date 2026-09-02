#ifndef HAARD_RESOLUTION_TABLE_H
#define HAARD_RESOLUTION_TABLE_H

#include <haard/haard.h>
#include <vector>

namespace haard {
    // What one node turned out to be, per record 0019.
    //
    // Two answers and not one, because a name has both: 'p' in 'p->area()' is
    // a 'Square*' and it is *that* parameter, and an emitter needs the second
    // as much as the first -- to write a local as a bare name, a field as
    // 'this->x', and a global as its module's name plus its own
    struct Resolution {
        // an index into THIS module's type table. INVALID_TYPE for a node
        // that is not an expression, and for one whose type could not be
        // worked out -- which was already reported where it happened
        u32 type;

        // the declaration this node names: which module it lives in and
        // which candidate it is. Both 0 when the node names none.
        //
        // A candidate and not a symbol, for record 0016's reason: a symbol is
        // a name and may hold several declarations, and a resolved name means
        // exactly one of them
        u32 module;
        u32 candidate;
    };

    // The answers the type phase worked out, kept instead of thrown away.
    //
    // Record 0019. The fourth table of a module, beside StringTable,
    // SymbolTable and TypeTable, and the same shape as all three: one flat
    // vector, index 0 a sentinel, nothing owned. A read past the end gives
    // back the sentinel rather than growing or failing, which is the rule
    // Ast::get_node already follows -- so asking about a node nothing wrote
    // is 'no type and no declaration' and never a crash.
    //
    // It is indexed by Ast node, densely. A map would be smaller for a file
    // that is mostly declarations and would cost the one discipline records
    // 0013 and 0016 both keep: a section of the blob is a resize() and a
    // memcpy, and nothing in it is a pointer.
    class ResolutionTable {
        public:
            ResolutionTable();

        public:
            // what this node is. Writing grows the vector to reach the node,
            // so nobody has to be told the tree's size
            void set_type(u32 node, u32 type);

            // which declaration this node names. Written by the typer at the
            // node that carries the NAME -- the identifier, or the right side
            // of a '.' -- and not at the call or the dot above it
            void set_declaration(u32 node, u32 module, u32 candidate);

            Resolution* get(u32 node);

            u32 get_count();

        private:
            void reach(u32 node);

        private:
            std::vector<Resolution> resolutions;
    };
}

#endif
