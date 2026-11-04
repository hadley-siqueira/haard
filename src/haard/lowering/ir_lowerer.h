#ifndef HAARD_IR_LOWERER_H
#define HAARD_IR_LOWERER_H

#include <haard/ir/ir_layout.h>
#include <haard/ir/ir_program.h>
#include <haard/ir/ir_type_translator.h>
#include <haard/type_table/type_builder.h>

#include <map>
#include <set>
#include <utility>

namespace haard {
    // a function's shape beyond its Haard signature (question 80)
    const u8 SHAPE_DESTINATION = 1;
    const u8 SHAPE_METHOD = 2;

    // What C++ writes for a struct without being asked, and the IR has to
    // write out (question 83). Each is a function, made the first time one
    // is asked for
    typedef enum StructorKind {
        // the base and the fields made ready -- given the value written on
        // them, or built with nothing -- then the 'init' chosen, if any
        STRUCTOR_CONSTRUCT,

        // member by member, for a struct with no copy 'init' of its own
        STRUCTOR_COPY,

        // the 'destroy' method, then the fields last to first, then the base
        STRUCTOR_DESTROY,

        // record 0031's assignment: destroy, then the copy 'init'; or field
        // by field when only a field says how
        STRUCTOR_ASSIGN,

        // destroy, then the move 'init' (Hadley, 2026-09-30)
        STRUCTOR_MOVE_ASSIGN,
    } StructorKind;

    // Record 0058: a closure is a function taking its environment first, and
    // the environment a struct of one pointer per variable it captures -- and
    // 'self' last, when its body reads the object of the method around it
    struct ClosureJob {
        u32 module;
        u32 node;
        IrObjectId function;
        IrTypeId holder;
        IrTypeId environment;
        std::vector<u32> captures;
        bool uses_self;
    };

    // Record 0058 section 8 and record 0071: a function named where a value
    // goes is a pair whose function is an adapter -- for a 'def' it drops
    // the environment, for a method the environment is the object, of class
    // 'from', and the method is called on it as every method is
    struct AdapterJob {
        u32 module;
        u32 candidate;
        IrObjectId function;
        IrTypeId from;
        bool method;
    };

    // a global a module's finaliser ends
    struct GlobalEnd {
        IrObjectId global;
        IrTypeId type;
    };

    struct StructorJob {
        u8 kind;
        IrTypeId type;

        // the 'init' a constructor ends in, as a candidate of the struct's
        // module, 0 for none
        u32 init;
        IrObjectId function;
    };

    // From a checked program to the IR (record 0077, stage 4).
    //
    // It reads what the C++ emitter reads -- the Ast after the sugar passes,
    // the ResolutionTable, every module's tables -- and writes the
    // structured form: an object per function and per global, each body by a
    // FunctionLowerer, then the verifier. 'finish' flattens it and verifies
    // again (questions 78 and 88), so what is left is a CFG ready to run.
    //
    // A global is a datum that starts zeroed, as C++'s static storage does,
    // and each module that has any gets a function giving them their values
    // in source order, run before 'main' in module order: the order the C++
    // emitter writes them in, so a value read before it is given is the same
    // zero on both paths.
    //
    // It is built one construct at a time, in the six steps record 0077
    // lists, and a construct it does not lower yet is refused by name --
    // 'not lowered yet: ...' -- never written as something else.
    class IrLowerer {
        public:
            IrLowerer();

        public:
            void set_compilation(Compilation* compilation);
            void set_target(const Target* target);
            void set_program(IrProgram* program);

            // the structured form, verified. False when something could not
            // be lowered, or the IR did not verify, with the reason in
            // get_error
            bool lower();

            // flatten, and the verifier again: what is left is a CFG ready to
            // run (questions 78 and 88)
            bool finish();

            const std::string& get_error();

        public:
            // for the FunctionLowerer
            Compilation* get_compilation();
            IrTypeTranslator* get_translator();
            const Target* get_target();

            // the global a binding at module level became, 0 when none
            IrObjectId global_of(u32 module, u32 candidate);

            // an object for a temporary a global reference is given
            IrObjectId global_temporary(IrTypeId type);

            // a string literal's bytes, as a read-only text: one object for
            // each text however often it is written
            IrObjectId text(const std::string& bytes);

            // record 0041: a symbol is the address of its entry, one entry
            // per name in the whole program, so comparing two is comparing
            // addresses; the entry holds the name's text, which 'as char*'
            // reads
            IrObjectId symbol(const std::string& name);

            // '__alloc' (a size, as a usize, to a pointer) and '__free', the
            // heap's natives, declared the first time one is asked for
            IrObjectId allocator();
            IrObjectId deallocator();

            // '__pow' and '__floor', over f64, what a float's '**' and '//'
            // come down to (record 0057): declared the first time one is
            // asked for, as C's 'pow' and 'floor' on the C++ path
            IrObjectId power_of_floats();
            IrObjectId floor_of_float();

            // the object a function's candidate became, 0 when none. Only
            // what is reachable is lowered (question 89): a function is
            // declared the first time one is asked for, and its body joins
            // the list still to lower
            IrObjectId function_of(u32 module, u32 candidate);

            // a 'vcall' through this slot is written, so every vtable that
            // has the slot gets its function (question 89): 'receiver' is the
            // class that introduced it, receiver_type's answer
            void use_slot(IrTypeId receiver, u32 slot);

            // SHAPE_DESTINATION and SHAPE_METHOD
            u8 shape_of(IrObjectId function);

            // the struct, class or union a method is declared in, as its
            // declaration's node; 0 for a free function
            u32 holder_of(u32 module, u32 candidate);
            IrTypeId type_of_declaration(u32 module, u32 declaration);

            // a type a value of which the lowering can hold: a scalar, a
            // pointer, a fixed array of those, and a struct of those -- a
            // class, a union, an enum and a tuple wait for steps 4 and 5
            bool lowerable(IrTypeId type);

            IrObjectId structor(StructorKind kind, IrTypeId type, u32 init);

            // A class's vtable (record 0077, stage 1): its descriptor, then
            // each slot's function, slot 0 the destructor. A datum with
            // relocations, made the first time a class is built, with the
            // descriptor -- name, size, base -- beside it. A slot no 'vcall'
            // goes through stays null, so a method nobody calls is never
            // lowered (question 89)
            IrObjectId vtable_of(IrTypeId type);

            // the slot a method of a class fills, -1 for one that fills none
            // ('init', a struct's method)
            i32 slot_of_method(u32 module, u32 candidate);

            // the class a method's object is passed as: the highest class
            // whose vtable has the method's slot, so every override of it
            // takes the same; the method's own class or struct otherwise
            IrTypeId receiver_type(u32 module, u32 candidate);

            // the same for the destructor, which is slot 0
            IrTypeId top_class(IrTypeId type);
            const StructorJob& get_structor(IrObjectId function);

            // a struct's own methods, as candidates of its module, 0 when it
            // has none: its 'destroy', the 'init' taking one of it, the one
            // taking a 'T&&', and the one that takes nothing at all
            u32 destroy_of(IrTypeId type);
            u32 copy_init_of(IrTypeId type);
            u32 move_init_of(IrTypeId type);
            u32 empty_init_of(IrTypeId type);

            // by name and number of parameters; for an 'init' of one, 'kind'
            // 1 asks for one taking this struct, 2 a 'T&&' of it, 0 anything
            u32 method_named(IrTypeId type, const std::string& name,
                             u32 parameters, u8 kind);
            bool has_init(IrTypeId type);

            // record 0067: a struct the compiler writes for a shape
            bool is_tuple(IrTypeId type);

            // a Haard type of module 'from' written in module 'into'
            // (record 0016)
            u32 translate_type(u32 into, u32 from, u32 type);

            // the declaration of the enum a variant is in: the scope that
            // holds its name, since a variant opens no scope of its own
            u32 enum_of_variant(u32 module, u32 candidate);

            // record 0058: a value of type 'A -> R' is '{env, call}'
            bool is_function_value(IrTypeId type);

            // the IR signature a value of type 'A -> R' is called with: the
            // environment, where an aggregate result is built, then each
            // parameter -- and whether it has that destination
            u32 signature_of_value(u32 module, u32 type, bool& destination);

            // the function a closure became, declared the first time it is
            // asked for with the class of the method around it ('holder',
            // what 'self' is), and its body lowered later
            IrObjectId closure_of(u32 module, u32 node, IrTypeId holder);
            const ClosureJob& closure_job(u32 module, u32 node);

            // the adapter that calls a function's candidate through a value
            // of type 'type' (of module 'at'): 'from' is the object's class
            // for a method, 0 for a 'def'
            IrObjectId adapter_of(u32 module, u32 candidate, IrTypeId from,
                                  u32 at, u32 type);

            // what a closure's body names from outside it, in source order:
            // locals and parameters of its module, never a global
            std::vector<u32> captures_of(u32 module, u32 closure,
                                         bool& uses_self);

            // record 0046: the 'init' of one parameter a value of 'given'
            // (a type of module 'caller') builds this struct from, the same
            // question Coercion::builds_from asks; 0 when there is none
            u32 init_from(IrTypeId type, u32 caller, u32 given);

            // through the base, the fields and an array's elements: whether
            // ending one runs anything, whether copying one calls an 'init',
            // whether one built with nothing runs anything, and the emitter's
            // two answers about assignment (record 0031)
            bool has_destructor(IrTypeId type);
            bool is_tagged(IrTypeId type);
            bool variants_ask(IrTypeId type, u8 asked);
            bool copies_by_init(IrTypeId type);
            bool constructs(IrTypeId type);
            bool declares_copy(IrTypeId type);
            bool assigns_by_field(IrTypeId type);

            // the IR type of a Haard type, and 0 when it has none (void)
            IrTypeId type_of(u32 module, u32 type);

            void fail(const std::string& message);

        private:
            bool declare_main();
            bool declare_function(u32 module, u32 node, u32 holder);
            bool drain();
            bool fill_vtables();
            IrObjectId descriptor_of(IrTypeId type);
            IrTypeId introducer_of(IrTypeId type, u32 slot);
            bool declare_globals();
            bool lower_globals();
            bool lower_entry();

            // a name no other object has: a clone or an overload of a
            // function, a global, shares one
            std::string unique(const std::string& name);
            bool verify(const std::string& when);
            bool is_generic(u32 module, u32 declaration);

        private:
            Compilation* compilation;
            const Target* target;
            IrProgram* program;
            IrTypeTranslator translator;
            TypeBuilder type_builder;

            // (module, candidate) to the function's object, and each body
            // still to lower as (module, node)
            std::map<std::pair<u32, u32>, u32> functions;
            std::vector<std::pair<u32, u32>> bodies;
            std::map<std::string, u32> names;

            // (module, candidate) to the global's object
            std::map<std::pair<u32, u32>, u32> globals;
            std::map<std::string, u32> texts;
            std::map<std::string, u32> symbols;
            std::map<u32, u8> shapes;

            // (kind, type, init) to the function, and every one made, in
            // order, its body lowered at the end
            std::map<std::vector<u32>, u32> made_structors;
            std::vector<StructorJob> structors;

            std::map<std::vector<u32>, u32> made_adapters;
            std::vector<AdapterJob> adapters;
            u32 next_adapter;
            std::map<std::pair<u32, u32>, u32> made_closures;
            std::vector<ClosureJob> closures;
            u32 next_closure;
            std::map<u32, u32> structor_index;

            std::map<u32, u32> vtables;
            std::map<u32, u32> descriptors;

            // what is lowered so far of 'bodies' and 'structors'; each
            // vtable's class, in order; the slots a 'vcall' goes through, as
            // (introducing class, slot); and the slots filled, as (vtable's
            // class, slot)
            u32 next_body;
            u32 next_structor;
            std::vector<u32> vtable_classes;
            std::set<std::pair<u32, u32>> used_slots;
            std::set<std::pair<u32, u32>> filled_slots;

            IrObjectId alloc_native;
            IrObjectId free_native;
            IrObjectId pow_native;
            IrObjectId floor_native;

            // record 0051: the function the program starts at when 'main'
            // takes one list, 0 when it does not, and that 'main's candidate
            IrObjectId entry_of_list;
            u32 entry_taking;

            std::string error;
    };
}

#endif
