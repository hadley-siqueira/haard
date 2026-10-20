#ifndef HAARD_IR_OPCODE_H
#define HAARD_IR_OPCODE_H

#include <haard/haard.h>

namespace haard {
    // The first instruction set (record 0077, stage 2, Hadley 2026-10-20).
    //
    // Every instruction carries Haard's own semantics and none is undefined:
    // 'add' wraps (question 1a), a shift by the width or more gives the
    // arithmetic result (2b), 'sdiv' by zero aborts and MIN / -1 is MIN (3a).
    // Each back end writes whatever check its machine needs for that.
    typedef enum IrOpcode {
        IR_OP_NONE,

        // constants
        IR_CONST,
        IR_NULL,
        IR_ADDR,
        IR_SIZE,

        // whole numbers, signedness in the opcode (question 79)
        IR_ADD,
        IR_SUB,
        IR_MUL,
        IR_SDIV,
        IR_UDIV,
        IR_SREM,
        IR_UREM,
        IR_SMULHI,
        IR_UMULHI,
        IR_AND,
        IR_OR,
        IR_XOR,
        IR_SHL,
        IR_LSHR,
        IR_ASHR,
        IR_NEG,
        IR_NOT,

        // floating point
        IR_FADD,
        IR_FSUB,
        IR_FMUL,
        IR_FDIV,
        IR_FNEG,

        // comparisons, giving a bool
        IR_EQ,
        IR_NE,
        IR_SLT,
        IR_SLE,
        IR_SGT,
        IR_SGE,
        IR_ULT,
        IR_ULE,
        IR_UGT,
        IR_UGE,
        IR_FEQ,
        IR_FNE,
        IR_FLT,
        IR_FLE,
        IR_FGT,
        IR_FGE,

        // conversions, the type written being the one converted to
        IR_TRUNC,
        IR_ZEXT,
        IR_SEXT,
        IR_SITOFP,
        IR_UITOFP,
        IR_FPTOSI,
        IR_FPTOUI,
        IR_FPEXT,
        IR_FPTRUNC,
        IR_PTRTOINT,
        IR_INTTOPTR,

        IR_SELECT,

        // memory: aggregates are never values, always slots (question 80)
        IR_SLOT,
        IR_LOAD,
        IR_STORE,
        IR_COPY,
        IR_FIELD,
        IR_INDEX,
        IR_UPCAST,
        IR_DOWNCAST,

        // calls
        IR_CALL,
        IR_CALL_INDIRECT,
        IR_VCALL,

        // barriers (question 85)
        IR_VOLATILE_LOAD,
        IR_VOLATILE_STORE,
        IR_FENCE,

        // the structured form (question 78)
        IR_BLOCK,
        IR_IF,
        IR_LOOP,
        IR_BREAK,
        IR_CONTINUE,

        // the CFG form
        IR_BR,
        IR_COND_BR,
        IR_SWITCH,

        // leaving
        IR_RET,
        IR_ABORT,

        IR_OP_COUNT,
    } IrOpcode;

    // Question 85: what an instruction does besides giving its result, in
    // one table every pass asks and none keeps a list of its own
    const u8 IR_EFFECT_READS = 1;
    const u8 IR_EFFECT_WRITES = 2;

    // it may abort the program, so it is never speculated: a division, a
    // float converted to a whole number
    const u8 IR_EFFECT_TRAPS = 4;

    // nothing crosses it, it is never removed, never run more or fewer
    // times, never speculated, and the WCET sees it: a timing instruction, a
    // volatile access, a transfer, and every call in the first version
    const u8 IR_EFFECT_BARRIER = 8;

    struct IrOpcodeInfo {
        const char* name;
        u8 effects;

        // it ends a block: nothing comes after it in the same block
        bool terminator;
    };

    const IrOpcodeInfo& opcode_info(IrOpcode opcode);
}

#endif
