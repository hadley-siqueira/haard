#ifndef HAARD_TYPE_TABLE_H
#define HAARD_TYPE_TABLE_H

#include <haard/haard.h>
#include <map>
#include <vector>

namespace haard {
    const u32 INVALID_TYPE = 0;

    // an array written with no length: 'T[]', which is its own type and not
    // 'T[0]'
    const u32 NO_LENGTH = 0xffffffff;

    typedef enum TypeKind {
        TYPE_NONE,
        TYPE_BUILTIN,
        TYPE_NAMED,
        TYPE_GENERIC,
        TYPE_POINTER,
        TYPE_REFERENCE,
        TYPE_ARRAY,
        TYPE_LIST,
        TYPE_HASH,
        TYPE_TUPLE,
        TYPE_FUNCTION,
    } TypeKind;

    // The thirteen the grammar knows, in the order the constructor seeds them,
    // so a builtin holds the same index in every module and the commonest
    // comparison in the language crosses a module boundary untranslated
    typedef enum BuiltinType {
        BUILTIN_U8, BUILTIN_U16, BUILTIN_U32, BUILTIN_U64,
        BUILTIN_I8, BUILTIN_I16, BUILTIN_I32, BUILTIN_I64,
        BUILTIN_F32, BUILTIN_F64,
        BUILTIN_BOOL, BUILTIN_VOID, BUILTIN_CHAR,
        BUILTIN_COUNT,
    } BuiltinType;

    struct Type {
        u8 kind;

        // the builtin's code, the CANDIDATE of a named or generic type, or
        // an array's length -- as a value and not as the expression node that
        // wrote it, or two '10's would be two types.
        //
        // A candidate and not a symbol: a symbol is a name and may hold
        // several declarations, and a type means one of them
        u32 subject;

        // which module 'subject' belongs to, for a named type. Record 0013
        // made the (module, symbol) pair transient for symbols; a type is
        // structural and has to write it down
        u32 module;

        u32 first_argument;
        u16 argument_count;
    };

    // The types of one module, each written once, per record 0016.
    //
    // The arguments live in a pool and not in a sibling chain, which is the one
    // place this family breaks the Ast's idiom on purpose: a node of a linked
    // list can only be in one chain, so '(i32, f32)' built twice would produce
    // two chains at two indices and the deduplication would fail exactly where
    // it is the point. A contiguous run compares as a unit.
    //
    // Index 0 is the sentinel, as everywhere else, and reads as 'no type'
    class TypeTable {
        public:
            TypeTable();

        public:
            u32 builtin(BuiltinType which);

            u32 pointer(u32 type);
            u32 reference(u32 type);
            u32 list(u32 type);

            // length is NO_LENGTH for 'T[]'
            u32 array(u32 type, u32 length);

            u32 hash(u32 key, u32 value);
            u32 tuple(const std::vector<u32>& elements);

            // the return type is the last argument. Record 0012 keeps it out
            // of what distinguishes two overloads, so resolution reads every
            // argument but the last one
            u32 function(const std::vector<u32>& parameters, u32 result);

            u32 named(u32 module, u32 candidate,
                      const std::vector<u32>& arguments);
            u32 generic(u32 module, u32 candidate);

        public:
            // What a reference names, and the type itself when it is not
            // one. Record 0018 amended 2026-09-06: **a reference IS the
            // thing it names**, which is C++'s semantics and the only one
            // that makes a 'T&' worth giving back from a function.
            //
            // Anywhere a *value* is what matters -- an operand, a subscript,
            // the target of an assignment -- this is what the type is.
            // Whether the read is allowed is a different question and belongs
            // to Coercion, because reading a reference is a **copy** and
            // record 0031 has something to say about those
            u32 value_of(u32 index);

            Type* get_type(u32 index);
            u32 get_argument(u32 index);
            u32 get_count();

            // the arguments of a type, copied out for a caller that wants them
            // as a list rather than as a run
            std::vector<u32> get_arguments(u32 index);

        private:
            u32 intern(TypeKind kind, u32 subject, u32 module,
                       const std::vector<u32>& arguments);

            bool same(u32 index, TypeKind kind, u32 subject, u32 module,
                      const std::vector<u32>& arguments);

        private:
            std::vector<Type> types;
            std::vector<u32> pool;

            // the hash of an entry, back to the entries carrying it. Built
            // while interning and never written down: it says nothing the
            // entries do not
            std::map<u32, std::vector<u32>> by_hash;
    };
}

#endif
