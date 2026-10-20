#ifndef HAARD_IR_PROGRAM_H
#define HAARD_IR_PROGRAM_H

#include <haard/ir/ir_class_table.h>
#include <haard/ir/ir_function.h>

#include <map>
#include <string>

namespace haard {
    typedef enum IrObjectKind {
        IR_OBJECT_NONE,
        IR_OBJECT_FUNCTION,
        IR_OBJECT_DATA,
    } IrObjectKind;

    // a datum's place that holds another object's address, filled in when
    // the object is placed: a vtable's slots, a pointer in a global
    struct IrRelocation {
        u32 offset;
        IrObjectId target;
    };

    // Question 81: everything that occupies memory is named, and an
    // instruction reaches it by this name and never by an address. Where it
    // goes is the placement table's business, at the end
    struct IrObject {
        u8 kind;
        std::string name;

        // a function's body, by index into the program's functions
        u32 function;

        // a function with no body, whose behaviour is known by its name: the
        // natives of 'std.low_io' (record 0030), which the interpreter and
        // each back end write
        bool native;

        // what a datum holds, its initial bytes in the target's order --
        // none when it starts zeroed -- and whether it may be written
        IrTypeId type;
        std::vector<u8> bytes;
        bool zeroed;
        bool read_only;
        std::vector<IrRelocation> relocations;
    };

    // a function's parameters and its result, 0 for none
    struct IrSignature {
        std::vector<IrTypeId> parameters;
        IrTypeId result;
    };

    // The whole program's IR (question 89): one type table, one class table,
    // the objects, the signatures and the functions' bodies
    class IrProgram {
        public:
            IrProgram();

        public:
            IrTypeTable* get_types();
            IrClassTable* get_classes();

            // interned, so two functions taking the same have one
            u32 signature(const std::vector<IrTypeId>& parameters,
                          IrTypeId result);
            const IrSignature& get_signature(u32 index);

            // a function's object and its empty body, in this form
            IrObjectId add_function(const std::string& name, u32 signature,
                                    IrForm form);

            IrObjectId add_native(const std::string& name, u32 signature);

            // the bytes at 'offset' in this datum hold 'target''s address
            void add_relocation(IrObjectId data, u32 offset,
                                IrObjectId target);

            IrObjectId add_data(const std::string& name, IrTypeId type,
                                const std::vector<u8>& bytes, bool read_only);
            IrObjectId add_zeroed(const std::string& name, IrTypeId type);

            // a read-only array of bytes ending in a zero: an 'abort's
            // message, a literal
            IrObjectId add_text(const std::string& name,
                                const std::string& text);

            IrObject& get_object(IrObjectId id);
            u32 get_object_count();
            IrFunction& get_function(IrObjectId id);

        private:
            IrTypeTable types;
            IrClassTable classes;

            std::vector<IrObject> objects;
            std::vector<IrFunction> functions;
            std::vector<IrSignature> signatures;
            std::map<std::vector<u32>, u32> signature_index;
    };
}

#endif
