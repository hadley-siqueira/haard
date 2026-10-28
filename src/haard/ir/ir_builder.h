#ifndef HAARD_IR_BUILDER_H
#define HAARD_IR_BUILDER_H

#include <haard/ir/ir_program.h>

namespace haard {
    // Writes instructions into one function of a program, at the end of a
    // block (record 0077, stage 2).
    //
    // Every instruction is given the place in the source it came from
    // (question 86): it is an argument of every call here, so nothing can be
    // made without one. In the structured form the regions nest: 'begin_if',
    // 'begin_loop' and 'begin_block' open one and move into its body, and
    // 'end' closes it and comes back to where it was opened.
    class IrBuilder {
        public:
            IrBuilder();

        public:
            void set_program(IrProgram* program);

            // starts writing a function's body: its entry in a CFG, its one
            // block in the structured form. Its parameters are made here
            void begin_function(IrObjectId function);
            IrValueId parameter(u32 which);

        public:
            IrValueId constant(IrTypeId type, u64 bits, IrPosition at);
            IrValueId null(IrPosition at);
            IrValueId address(IrObjectId object, IrPosition at);

            // 'result' is the integer the size is given as: the target's
            // 'usize', which this target-independent IR is told
            IrValueId size(IrTypeId type, IrTypeId result, IrPosition at);

            // an arithmetic, a comparison (giving a bool) or a bitwise
            // operation, with the operands' type written
            IrValueId binary(IrOpcode opcode, IrTypeId type, IrValueId left,
                             IrValueId right, IrPosition at);

            // 'neg', 'not', 'fneg'
            IrValueId unary(IrOpcode opcode, IrTypeId type, IrValueId operand,
                            IrPosition at);

            // to the type written
            IrValueId convert(IrOpcode opcode, IrTypeId to, IrValueId operand,
                              IrPosition at);

            IrValueId select(IrTypeId type, IrValueId condition,
                             IrValueId chosen, IrValueId otherwise,
                             IrPosition at);

        public:
            IrValueId slot(IrTypeId type, IrPosition at);

            // a slot at the top of the function's body, after the slots
            // already there, wherever the builder is writing: question 13
            // gives every frame a fixed size, so the lowering puts every local
            // and every temporary there, as clang does
            IrValueId entry_slot(IrTypeId type, IrPosition at);

            // a bool slot at the top of the body that starts false, written
            // there too: what says a temporary made on one path only was made
            // (question 83)
            IrValueId entry_flag(IrPosition at);
            IrValueId load(IrTypeId type, IrValueId from, IrPosition at);
            void store(IrTypeId type, IrValueId value, IrValueId to,
                       IrPosition at);
            void copy(IrTypeId type, IrValueId to, IrValueId from,
                      IrPosition at);
            IrValueId field(IrTypeId aggregate, IrValueId of, u32 which,
                            IrPosition at);
            IrValueId index(IrTypeId element, IrValueId of, IrValueId which,
                            IrPosition at);

            // both of this whole-number type
            void check_index(IrTypeId type, IrValueId index, IrValueId length,
                             IrPosition at);
            IrValueId upcast(IrTypeId from, IrTypeId to, IrValueId of,
                             IrPosition at);
            IrValueId downcast(IrTypeId from, IrTypeId to, IrValueId of,
                               IrPosition at);

        public:
            // 0 when the function gives nothing back
            IrValueId call(IrObjectId function,
                           const std::vector<IrValueId>& arguments,
                           IrPosition at);
            IrValueId call_indirect(u32 signature, IrValueId function,
                                    const std::vector<IrValueId>& arguments,
                                    IrPosition at);

            // the receiver is the first argument (question 83)
            IrValueId vcall(IrTypeId type, u32 slot, u32 signature,
                            const std::vector<IrValueId>& arguments,
                            IrPosition at);

            IrValueId volatile_load(IrTypeId type, IrValueId from,
                                    IrPosition at);
            void volatile_store(IrTypeId type, IrValueId value, IrValueId to,
                                IrPosition at);
            void fence(IrPosition at);

        public:
            // the structured form
            IrInstId begin_block(IrPosition at);
            IrInstId begin_if(IrValueId condition, IrPosition at);
            void begin_else();

            // 'bound' is IR_NO_BOUND when none is known (question 84)
            IrInstId begin_loop(u32 bound, IrPosition at);
            void end();

            // whether the block being written ends in a terminator already,
            // so nothing more may be written in it
            bool ended();
            void break_to(IrInstId region, IrPosition at);
            void continue_to(IrInstId region, IrPosition at);

            // the CFG form
            // a block and its parameters, made together so they are one run
            // of the pool
            IrBlockId new_block(const std::vector<IrTypeId>& parameters);
            IrValueId block_parameter(IrBlockId block, u32 which);
            void set_block(IrBlockId block);
            void br(IrBlockId target, const std::vector<IrValueId>& arguments,
                    IrPosition at);
            void cond_br(IrValueId condition, IrBlockId chosen,
                         IrBlockId otherwise, IrPosition at);

            // each case a constant of the value's type and where it goes
            void switch_on(IrTypeId type, IrValueId value, IrBlockId otherwise,
                           const std::vector<std::pair<u64, IrBlockId>>& cases,
                           IrPosition at);

            // leaving
            void ret(IrPosition at);
            void ret(IrTypeId type, IrValueId value, IrPosition at);
            void abort(IrObjectId message, IrPosition at);

        private:
            // appends one instruction to the current block, with a result of
            // this type when 'result' is not 0
            IrInstId emit(IrOpcode opcode, IrTypeId type, u32 a, u32 b, u32 c,
                          u32 extra, const std::vector<u32>& pool,
                          IrTypeId result, IrPosition at);

            // links an instruction made by 'emit' elsewhere: after 'after' in
            // its block, or first when 'after' is 0
            void move_after(IrInstId inst, IrBlockId block, IrInstId after);

            IrBlockId add_block(IrInstId region);
            IrValueId add_value(IrValueKind kind, IrTypeId type, u32 owner);
            IrValueId result_of(IrInstId inst);
            IrTypeId pointer();
            IrFunction& function();

        private:
            IrProgram* program;
            IrObjectId current;
            IrBlockId block;

            // the last slot 'entry_slot' put at the top of the body
            IrInstId last_entry_slot;

            // the regions open, innermost last, and the block each was
            // opened in
            std::vector<std::pair<IrInstId, IrBlockId>> open;
    };
}

#endif
