#ifndef HAARD_SYMBOL_TABLE_H
#define HAARD_SYMBOL_TABLE_H

#include <haard/haard.h>
#include <vector>

namespace haard {
    // What a candidate declares. It is kept on the candidate and not derived
    // from its node, because record 0013 lets a lookup return candidates from
    // a dependency and a filter over them must not reach into another module's
    // Ast to ask. Keeping it here is also what makes a 'def foo' next to a
    // 'let foo' representable, and nothing that cannot be represented can be
    // reported
    typedef enum SymbolKind {
        SYMBOL_NONE,
        SYMBOL_FUNCTION,
        SYMBOL_CLASS,
        SYMBOL_STRUCT,
        SYMBOL_ENUM,
        SYMBOL_UNION,
        SYMBOL_FIELD,
        SYMBOL_VARIABLE,
        SYMBOL_PARAM,
        SYMBOL_GENERIC,
    } SymbolKind;

    // A name inside one scope, not a declaration. Record 0012 made a name map
    // to a list of declarations rather than to one, and this is that list's
    // head: one Symbol per distinct name per scope, and the overloads hang off
    // it as candidates
    struct Symbol {
        u32 name;               // index into the module's StringTable
        u32 candidates;         // head of the candidate list
        u32 sibling_or_next;    // next name in the same scope
    };

    struct Candidate {
        u8 kind;
        u32 ast_node;           // the declaration, in this module's Ast
        u32 next_candidate;     // next declaration of the same name

        // what this declaration is, in the module's type table. A function
        // holds its whole signature here, which is what overload resolution
        // reads: record 0012 makes the return type not distinguish two
        // overloads, and record 0016 puts the return last so it is skipped.
        //
        // INVALID_TYPE when nothing wrote one and nothing could infer it --
        // 'let a = 1' stays 0 until the inference phase exists
        u32 type;

        // the type this declaration derives from, for a class, a struct or a
        // union, and 0 for everything else and for a class with no base.
        //
        // A type and not a candidate, so that 'class Grid(Matrix<i32>)' needs
        // no special case: what a class derives from may be an instantiation.
        //
        // One and not a list: Hadley settled single inheritance on 2026-09-02,
        // which is what keeps record 0018's ranking a number along a chain.
        // Interfaces, if they arrive, are a second axis and not this field
        u32 super;
    };

    struct Scope {
        u32 parent;             // enclosing scope, 0 at the module scope
        u32 symbols;            // head of the name list
        u32 owner;              // the node that opened it, 0 at the module
    };

    // The symbols of one module, per record 0013.
    //
    // Three flat vectors threaded by index, which is the Ast's idiom applied
    // twice: a scope's symbols are a children list walked by sibling_or_next,
    // and a symbol's candidates are the same pair one level down. So loading a
    // module stays resize() plus one memcpy per section, and nothing here owns
    // anything.
    //
    // Index 0 is a sentinel in each vector, as it is in the Ast, so that 0
    // means 'none' everywhere: a scope with no parent, a name with no next.
    // The module scope is therefore 1 and is opened by the constructor.
    //
    // The lists are appended to at the end and never sorted. Source order is
    // not a nicety here -- record 0009 resolves a bare name to the first
    // declaration that answers, so the order of the candidate chain decides
    // which declaration a program means
    class SymbolTable {
        public:
            SymbolTable();

        public:
            // the scope every module-level declaration goes in, and the last
            // stop of a lookup before it starts asking the imports
            u32 get_module_scope();

            u32 open_scope(u32 parent, u32 owner);

            // adds a candidate for this name in this scope, creating the
            // Symbol the first time the name is seen there. Gives back the
            // symbol, which is the same one for every overload
            u32 declare(u32 scope, u32 name, SymbolKind kind, u32 ast_node);

            // the symbol for this name in this scope alone, 0 when the scope
            // does not declare it. Walking outward is the resolver's job:
            // record 0012 makes the enclosing scopes contribute to one
            // candidate set for functions while a variable stops at the first,
            // and that rule reads the kind of what this gives back
            u32 find(u32 scope, u32 name);

            void set_candidate_type(u32 candidate, u32 type);
            void set_candidate_super(u32 candidate, u32 super);

            // the scope a declaration opened, 0 when it opened none. The way
            // from a class to its members, which is what a field access walks
            u32 scope_owned_by(u32 node);

            // the candidate a declaration node belongs to, 0 when none does.
            // The way from a scope's owner back to the declaration's symbol
            // side, which is what 'this' and a method call both need
            u32 candidate_of(u32 node);

        public:
            Scope* get_scope(u32 index);
            Symbol* get_symbol(u32 index);
            Candidate* get_candidate(u32 index);

            u32 get_scope_count();
            u32 get_symbol_count();
            u32 get_candidate_count();

        private:
            std::vector<Scope> scopes;
            std::vector<Symbol> symbols;
            std::vector<Candidate> candidates;
    };
}

#endif
