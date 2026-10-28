#ifndef HAARD_IR_LOWERER_H
#define HAARD_IR_LOWERER_H

#include <haard/ir/ir_program.h>
#include <haard/ir/ir_type_translator.h>

#include <map>
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

            // a string literal's bytes, as a read-only text: one object for
            // each text however often it is written
            IrObjectId text(const std::string& bytes);

            // '__alloc' (a size, as a usize, to a pointer) and '__free', the
            // heap's natives, declared the first time one is asked for
            IrObjectId allocator();
            IrObjectId deallocator();

            // the object a function's candidate became, 0 when none
            IrObjectId function_of(u32 module, u32 candidate);

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
            const StructorJob& get_structor(IrObjectId function);

            // a struct's own methods, as candidates of its module, 0 when it
            // has none: its 'destroy', the 'init' taking one of it, the one
            // taking a 'T&&', and the one that takes nothing at all
            u32 destroy_of(IrTypeId type);
            u32 copy_init_of(IrTypeId type);
            u32 move_init_of(IrTypeId type);
            u32 empty_init_of(IrTypeId type);
            bool has_init(IrTypeId type);

            // through the base, the fields and an array's elements: whether
            // ending one runs anything, whether copying one calls an 'init',
            // whether one built with nothing runs anything, and the emitter's
            // two answers about assignment (record 0031)
            bool has_destructor(IrTypeId type);
            bool copies_by_init(IrTypeId type);
            bool constructs(IrTypeId type);
            bool declares_copy(IrTypeId type);
            bool assigns_by_field(IrTypeId type);

            // the IR type of a Haard type, and 0 when it has none (void)
            IrTypeId type_of(u32 module, u32 type);

            void fail(const std::string& message);

        private:
            bool declare_functions();
            bool declare_function(u32 module, u32 node, u32 holder);
            bool lower_structors();
            u32 method_named(IrTypeId type, const std::string& name,
                             u32 parameters, u8 kind);
            bool declare_globals();
            bool lower_bodies();
            bool lower_globals();

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

            // (module, candidate) to the function's object, and each body
            // still to lower as (module, node)
            std::map<std::pair<u32, u32>, u32> functions;
            std::vector<std::pair<u32, u32>> bodies;
            std::map<std::string, u32> names;

            // (module, candidate) to the global's object
            std::map<std::pair<u32, u32>, u32> globals;
            std::map<std::string, u32> texts;
            std::map<u32, u8> shapes;

            // (kind, type, init) to the function, and every one made, in
            // order, its body lowered at the end
            std::map<std::vector<u32>, u32> made_structors;
            std::vector<StructorJob> structors;
            std::map<u32, u32> structor_index;

            IrObjectId alloc_native;
            IrObjectId free_native;

            std::string error;
    };
}

#endif
