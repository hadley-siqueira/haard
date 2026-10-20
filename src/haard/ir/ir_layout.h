#ifndef HAARD_IR_LAYOUT_H
#define HAARD_IR_LAYOUT_H

#include <haard/ir/ir_type_table.h>
#include <haard/ir/target.h>

#include <set>

namespace haard {
    struct IrLayout {
        u32 size;
        u32 align;

        // The Itanium ABI's dsize: the size without the tail padding. A
        // class derived from this one starts its own fields here when this
        // one is not POD for the purpose of layout, and at 'size' when it is
        u32 data_size;

        // no data at all: a struct with no fields whose bases are empty
        // too. It still takes one byte alone, and none as a base
        bool empty;

        // a class: the pointer to its vtable is at 0, its own or its base's
        bool dynamic;

        // whether an empty struct sits anywhere inside, which is all the
        // rule about two of them at one address has to look at
        bool holds_an_empty;

        u32 base_offset;

        // one per field, in the aggregate's order
        std::vector<u32> offsets;
    };

    // Sizes, alignments and offsets for one target (record 0077, stage 1).
    //
    // Declaration order, never reordered, every scalar aligned to its size
    // (question 80). Inheritance follows the Itanium C++ ABI, which is how
    // g++ lays out the C++ the emitter writes, on x86-64 and ARM64 (Hadley,
    // 2026-10-20): a class's vtable pointer first, shared with a class base;
    // a base that is not POD for the purpose of layout lending its tail
    // padding; an empty base taking no bytes, unless that would put two
    // empty subobjects of one type at one address. Single inheritance and no
    // virtual base, which is all Haard has, so none of the rest of the ABI's
    // allocation algorithm applies.
    //
    // Chosen over a simpler rule of our own so that g++ can check every
    // layout: tests/ir_layout compiles the emitted C++ and compares.
    class LayoutTable {
        public:
            LayoutTable();

        public:
            void set_target(const Target* target);
            void set_types(IrTypeTable* types);

            // every aggregate of the table. False at the first one that
            // holds itself by value, with the reason in get_error
            bool lay_out_all();

            u32 size_of(IrTypeId type);
            u32 align_of(IrTypeId type);

            // an aggregate's, once laid out
            const IrLayout* get_layout(IrTypeId type);

            const std::string& get_error();

        private:
            // an aggregate and, first, everything it holds by value
            bool lay_out(IrTypeId type);
            bool lay_out_held(IrTypeId type);
            void lay_out_record(IrTypeId type, IrLayout& layout);
            void lay_out_union(IrTypeId type, IrLayout& layout);

            // where a base or a field of this type goes, from 'at' on: the
            // first offset, a multiple of its alignment, at which no empty
            // struct inside it lands on one of the same type already placed
            u32 place(IrTypeId type, u32 at,
                      std::set<std::pair<u32, u32>>& taken);

            // every empty struct inside a value of this type at 'at', as
            // (aggregate, offset)
            void empties_of(IrTypeId type, u32 at,
                            std::vector<std::pair<u32, u32>>& found);

            bool holds_an_empty(IrTypeId type);

            u32 align_up(u32 value, u32 alignment);

        private:
            const Target* target;
            IrTypeTable* types;

            // by aggregate index; 'states' says not yet (0), being laid out
            // (1) and done (2), and a 1 met again is a type holding itself
            std::vector<IrLayout> layouts;
            std::vector<u8> states;

            std::string error;
    };
}

#endif
