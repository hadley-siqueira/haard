#ifndef HAARD_IR_FUNCTION_H
#define HAARD_IR_FUNCTION_H

#include <haard/ir/ir_opcode.h>
#include <haard/ir/ir_type_table.h>

#include <vector>

namespace haard {
    // Question 91: ids of four bytes, one struct per kind, so the C++
    // compiler refuses one given for another. Index 0 is 'none' in each
    struct IrInstId {
        u32 index;
    };

    struct IrBlockId {
        u32 index;
    };

    struct IrValueId {
        u32 index;
    };

    struct IrObjectId {
        u32 index;
    };

    // Question 86: where in the source an instruction came from, the module
    // and the byte offset, as tokens and the logger keep it
    struct IrPosition {
        u32 module;
        u32 offset;
    };

    const u32 IR_NO_OFFSET = 0xffffffff;

    // a loop with no bound written (question 84)
    const u32 IR_NO_BOUND = 0xffffffff;

    typedef enum IrForm {
        IR_FORM_STRUCTURED,
        IR_FORM_CFG,
    } IrForm;

    typedef enum IrValueKind {
        IR_VALUE_NONE,
        IR_VALUE_PARAMETER,
        IR_VALUE_RESULT,
        IR_VALUE_BLOCK_PARAMETER,
    } IrValueKind;

    // Every local is a slot (question 82), so a value is what an
    // instruction gives back, a parameter of the function, or a parameter of
    // a CFG block
    struct IrValue {
        u8 kind;
        IrTypeId type;

        // the parameter's position, the instruction, the block
        u32 owner;
    };

    // One instruction, of a fixed size (question 91). What each field means
    // depends on the opcode:
    //
    //   type      what is written on it: the operand type of an arithmetic
    //             or a comparison, the type converted to, the type loaded,
    //             stored, copied or held by a slot, the aggregate of a
    //             'field', the element of an 'index', the class of a
    //             'vcall', the type an 'upcast' or 'downcast' starts from
    //   a, b, c   value operands, in the order they are written; for an
    //             'if' 'c' is its 'else' block, for a 'loop' 'c' its bound
    //   extra     a constant's index, an object, a field's position, the
    //             type an 'upcast' or 'downcast' reaches, a signature, a
    //             vtable slot, a region's body, a 'break's region, a target
    //   pool      the rest, in the function's operand pool: a call's
    //             arguments, a 'br's block arguments, a 'switch's cases
    struct IrInst {
        u8 opcode;
        IrTypeId type;
        IrValueId result;
        u32 a;
        u32 b;
        u32 c;
        u32 extra;
        u32 pool_start;
        u32 pool_count;

        // the block it is in, and its neighbours there: an index-linked
        // list, so inserting and removing never moves anything
        IrBlockId block;
        IrInstId previous;
        IrInstId next;
    };

    struct IrBlock {
        IrInstId first;
        IrInstId last;

        // the region that owns it, 0 for the body of a structured function
        // and for every block of a CFG
        IrInstId region;

        // a CFG block's parameters, as values in the pool
        u32 pool_start;
        u32 pool_count;

        // Question 84 in the CFG form, written by flatten: a loop's header
        // carries the loop's bound, and every block knows the innermost
        // loop it is in -- a header the loop it heads, and 'outer' the one
        // around that. What the interpreter counts a loop's turns by, and
        // what the WCET will read
        bool header;
        u32 bound;
        IrBlockId loop;
        IrBlockId outer;
    };

    // A function's IR, in one form at a time (question 78). Everything is
    // reached by id and nothing holds a pointer into these vectors, which may
    // move whenever something is added (the Ast's lesson)
    class IrFunction {
        public:
            IrFunction();

        public:
            IrObjectId object;
            u32 signature;
            IrForm form;

            // question 57 decides what real-time code is; until then a
            // function is marked by whoever builds it
            bool real_time;

            // a structured function's body, or a CFG's entry
            IrBlockId body;

            // a CFG's blocks in the order they are printed, the entry first
            std::vector<IrBlockId> order;

            std::vector<IrInst> instructions;
            std::vector<IrBlock> blocks;
            std::vector<IrValue> values;
            std::vector<u32> pool;
            std::vector<u64> constants;

            // the side tables, by instruction id (question 86)
            std::vector<IrPosition> positions;

        public:
            IrInst& get(IrInstId id);
            IrBlock& get(IrBlockId id);
            IrValue& get(IrValueId id);

            IrValueId parameter(u32 which);

            // an instruction's pool, copied out
            std::vector<u32> pool_of(IrInstId id);
            std::vector<u32> parameters_of(IrBlockId id);
    };
}

#endif
