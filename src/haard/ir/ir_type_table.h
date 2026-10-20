#ifndef HAARD_IR_TYPE_TABLE_H
#define HAARD_IR_TYPE_TABLE_H

#include <haard/haard.h>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace haard {
    // An IR type, by index into the program's IrTypeTable. A struct of its
    // own and not a bare u32 (question 91), so the C++ compiler refuses one
    // given where a Haard type index was meant. Index 0 is 'no type'
    struct IrTypeId {
        u32 index;
    };

    const u32 IR_SPACE_GENERIC = 0;

    typedef enum IrTypeKind {
        IR_TYPE_NONE,

        // Question 79: an integer carries its width and nothing else;
        // signedness is in the instruction. 'b' and not 'i', so one is never
        // read as Haard's signed i32
        IR_B8,
        IR_B16,
        IR_B32,
        IR_B64,
        IR_BOOL,
        IR_F32,
        IR_F64,

        // Question 80: opaque, the pointee written on the instruction, with
        // an address space (generic unless a later answer to 35 says more)
        IR_PTR,

        // The aggregates. Never values (question 80): they are what a slot,
        // an object, 'field', 'index' and 'size T' are about
        IR_ARRAY,
        IR_STRUCT,
        IR_CLASS,
        IR_UNION,
    } IrTypeKind;

    struct IrType {
        u8 kind;

        // a pointer's address space, an array's length, an aggregate's index
        // in the aggregate list
        u32 subject;

        // an array's element
        IrTypeId element;
    };

    struct IrField {
        std::string name;
        IrTypeId type;

        // the field's declaration as a candidate of the aggregate's module,
        // 0 for one the compiler made: a tuple's element, an enum's tag
        u32 candidate;
    };

    // A struct, a class or a union: by identity and never by shape, since
    // two declarations with the same fields are two types. A tuple is the
    // exception and the IrTypeTranslator interns those by shape itself
    struct IrAggregate {
        std::string name;

        // a struct's or a class's base, index 0 when it has none
        IrTypeId base;
        std::vector<IrField> fields;

        // The Itanium C++ ABI's 'POD for the purpose of layout' (Hadley,
        // 2026-10-20), as the emitted C++ makes it: a struct with no base,
        // no 'init', no 'destroy', no field given a value and no reference
        // field. Only a base asks: one that is not POD lends its tail
        // padding to the class derived from it
        bool pod_for_layout;

        // where it came from: a declaration (module and Ast node), or a
        // Haard type of that module for one the compiler made (a tuple, a
        // function value). What --emit-ir and the layout tests name it by
        u32 module;
        u32 declaration;
        u32 source_type;
    };

    // The IR's types, one table for the whole program (question 89).
    //
    // Scalars are seeded in a fixed order, so 'b32' is the same index in
    // every program; a pointer and an array are interned by what they are
    // made of; an aggregate is a new entry each time it is asked for, and
    // its fields are filled in after, which is what lets a struct hold a
    // pointer to itself.
    class IrTypeTable {
        public:
            IrTypeTable();

        public:
            // IR_B8 to IR_F64
            IrTypeId scalar(IrTypeKind kind);

            // the integer of this many bytes: 1, 2, 4 or 8
            IrTypeId integer(u32 bytes);

            IrTypeId pointer(u32 space);
            IrTypeId array(IrTypeId element, u32 length);

            // a new, empty struct, class or union
            IrTypeId aggregate(IrTypeKind kind, const std::string& name);

        public:
            const IrType* get_type(IrTypeId type);
            IrAggregate* get_aggregate(IrTypeId type);
            bool is_aggregate(IrTypeId type);
            u32 get_count();

            // as --emit-ir writes it: 'b32', 'ptr', 'b8[4]', an aggregate
            // by its name
            std::string name_of(IrTypeId type);

        private:
            IrTypeId add(IrTypeKind kind, u32 subject, IrTypeId element);

        private:
            std::vector<IrType> types;
            std::vector<IrAggregate> aggregates;

            std::map<u32, u32> pointers;
            std::map<std::pair<u32, u32>, u32> arrays;
    };
}

#endif
