#ifndef HAARD_IR_VERIFIER_H
#define HAARD_IR_VERIFIER_H

#include <haard/ir/ir_program.h>

#include <set>

namespace haard {
    struct IrVerifierError {
        IrObjectId function;
        IrInstId inst;
        std::string message;
    };

    // Whether the IR is well formed (question 88): never whether the program
    // is right, which is the interpreter's question. Always on, after the
    // lowering and after every pass; a failure is a bug of the compiler.
    //
    // It checks, per function:
    //
    //   - the form (78): a structured function has no 'br', 'cond_br' or
    //     'switch', a 'break' or 'continue' leaves a region around it and a
    //     'continue' a loop; a CFG has no region, every block ends in one
    //     terminator, its targets are its blocks and nothing jumps to the
    //     entry. In both, nothing follows a terminator in its block
    //   - the types: what each opcode takes and gives, the fields and the
    //     classes it names (80)
    //   - that every use is dominated by its definition: a scope rule in the
    //     structured form (stage 2, choice 4), the dominator tree in a CFG
    //   - the calls: arity and types against the signature, a 'vcall''s
    //     class and slot (83)
    //   - the objects an instruction names, an 'abort''s message read-only
    //   - the side tables: every instruction has a position (86), every loop
    //     of real-time code a bound (84)
    //
    // That every opcode has its effects (85) is checked when the compiler is
    // built: a static_assert on the table
    class IrVerifier {
        public:
            IrVerifier();

        public:
            void set_program(IrProgram* program);

            // false when anything is wrong, with every error in get_errors
            bool verify();

            const std::vector<IrVerifierError>& get_errors();

        private:
            void verify_function(IrObjectId id);
            void verify_inst(IrInstId id);
            void verify_types(IrInstId id);
            void verify_call(IrInstId id, const std::vector<u32>& arguments,
                             u32 signature);
            void verify_jump(IrInstId id);
            void verify_structured_block(IrBlockId block);
            void verify_cfg();

            // whether a value may be used by this instruction
            bool is_visible(u32 value, IrInstId use);
            bool comes_before(IrInstId first, IrInstId second);
            bool dominates(IrBlockId first, IrBlockId second);
            void compute_dominators();
            std::vector<IrBlockId> successors(IrBlockId block);

            // a value operand: present, and of this type when 'type' is not 0
            bool operand(IrInstId id, u32 value, const char* which,
                         IrTypeId type);
            bool is_object(u32 index, u8 kind);
            bool is_derived(IrTypeId derived, IrTypeId base);

            bool is_integer(IrTypeId type);
            bool is_float(IrTypeId type);
            bool is_pointer(IrTypeId type);
            bool is_scalar(IrTypeId type);
            u32 width(IrTypeId type);

            IrTypeId type_of(u32 value);
            std::string name(IrTypeId type);
            void report(IrInstId id, const std::string& message);

        private:
            IrProgram* program;
            IrObjectId current;
            IrFunction* function;

            // a CFG's blocks, and the blocks that dominate each, by block
            // index: the plain iterative sets, which is all a verifier needs
            std::set<u32> in_order;
            std::vector<std::set<u32>> dominators;

            std::vector<IrVerifierError> errors;
    };
}

#endif
