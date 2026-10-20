#ifndef HAARD_IR_CLASS_TABLE_H
#define HAARD_IR_CLASS_TABLE_H

#include <haard/ast_query/ast_query.h>
#include <haard/compilation/compilation.h>
#include <haard/ir/ir_type_table.h>

#include <map>

namespace haard {
    // One entry of a vtable: the method that fills it in this class
    struct IrSlot {
        std::string name;

        // the method, as a candidate of its module; 0 for the destructor of
        // a class that writes no 'destroy', which is empty
        u32 module;
        u32 candidate;

        // the class whose method fills it, which is this class or one above
        IrTypeId implemented_by;
    };

    struct IrClass {
        IrTypeId type;

        // slot 0 is the destructor. The entry before it, pointing to the
        // class's descriptor, is not a slot: it is the descriptor's place,
        // where the Itanium ABI keeps its RTTI, and no method moves for it
        std::vector<IrSlot> slots;
    };

    // Every class's vtable (record 0077, stage 1, Hadley 2026-10-20).
    //
    // Slot 0 is the destructor, always virtual (record 0026). Then the
    // base's slots in its order, an override taking the slot of what it
    // overrides -- the same name and the same parameters, record 0020's rule,
    // with the return left out as record 0012 leaves it out -- and then the
    // class's new methods in the order it declares them. 'init' takes no
    // slot: construction always calls it directly (question 83). A struct's
    // methods are never virtual (record 0065), so a class derived from a
    // struct starts a vtable of its own.
    //
    // This is not the Itanium vtable the emitted C++ has. That one has two
    // destructor entries and a slot for 'init', because the emitter writes
    // every method of a class 'virtual'; neither means anything here.
    class IrClassTable {
        public:
            IrClassTable();

        public:
            void set_compilation(Compilation* compilation);
            void set_types(IrTypeTable* types);

            // every class aggregate of the type table
            void build();

            // nullptr when the type is not a class
            const IrClass* get_class(IrTypeId type);

            // the slot a method fills in this class, -1 when it fills none:
            // what a 'vcall' is lowered to
            i32 slot_of(IrTypeId type, u32 module, u32 candidate);

        private:
            void build_class(IrTypeId type);

            // whether two methods take the same parameters. A type index
            // means nothing outside its module (record 0016), so the types
            // are compared by what they are: a named type by the module and
            // the candidate that declare it, which is the same from anywhere
            bool same_parameters(u32 module, u32 candidate, u32 other_module,
                                 u32 other);
            bool same_type(u32 module, u32 type, u32 other_module, u32 other);

            bool is_generic(u32 module, u32 declaration);

        private:
            Compilation* compilation;
            IrTypeTable* types;

            std::vector<IrClass> classes;

            // a class's IR type index to its entry in 'classes'
            std::map<u32, u32> by_type;
    };
}

#endif
