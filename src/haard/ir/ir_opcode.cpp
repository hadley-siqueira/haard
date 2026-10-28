#include <haard/ir/ir_opcode.h>

using namespace haard;

static const u8 READS = IR_EFFECT_READS;
static const u8 WRITES = IR_EFFECT_WRITES;
static const u8 TRAPS = IR_EFFECT_TRAPS;
static const u8 BARRIER = IR_EFFECT_BARRIER;

// in the enum's order; tests/ir checks that every opcode has its entry
static const IrOpcodeInfo OPCODES[] = {
    {"none", 0, false},

    {"const", 0, false},
    {"null", 0, false},
    {"addr", 0, false},
    {"size", 0, false},

    {"add", 0, false},
    {"sub", 0, false},
    {"mul", 0, false},
    {"sdiv", TRAPS, false},
    {"udiv", TRAPS, false},
    {"srem", TRAPS, false},
    {"urem", TRAPS, false},
    {"smulhi", 0, false},
    {"umulhi", 0, false},
    {"and", 0, false},
    {"or", 0, false},
    {"xor", 0, false},
    {"shl", 0, false},
    {"lshr", 0, false},
    {"ashr", 0, false},
    {"neg", 0, false},
    {"not", 0, false},

    {"fadd", 0, false},
    {"fsub", 0, false},
    {"fmul", 0, false},
    {"fdiv", 0, false},
    {"fneg", 0, false},

    {"eq", 0, false},
    {"ne", 0, false},
    {"slt", 0, false},
    {"sle", 0, false},
    {"sgt", 0, false},
    {"sge", 0, false},
    {"ult", 0, false},
    {"ule", 0, false},
    {"ugt", 0, false},
    {"uge", 0, false},
    {"feq", 0, false},
    {"fne", 0, false},
    {"flt", 0, false},
    {"fle", 0, false},
    {"fgt", 0, false},
    {"fge", 0, false},

    {"trunc", 0, false},
    {"zext", 0, false},
    {"sext", 0, false},
    {"sitofp", 0, false},
    {"uitofp", 0, false},
    {"fptosi", TRAPS, false},
    {"fptoui", TRAPS, false},
    {"fpext", 0, false},
    {"fptrunc", 0, false},
    {"ptrtoint", 0, false},
    {"inttoptr", 0, false},

    {"select", 0, false},

    {"slot", 0, false},
    {"load", READS, false},
    {"store", WRITES, false},
    {"copy", READS | WRITES, false},
    {"field", 0, false},
    {"index", 0, false},
    {"check_index", TRAPS, false},
    {"upcast", 0, false},
    {"downcast", 0, false},

    {"call", READS | WRITES | BARRIER, false},
    {"call_indirect", READS | WRITES | BARRIER, false},
    {"vcall", READS | WRITES | BARRIER, false},

    {"volatile_load", READS | BARRIER, false},
    {"volatile_store", WRITES | BARRIER, false},
    {"fence", BARRIER, false},

    {"block", 0, false},
    {"if", 0, false},
    {"loop", 0, false},
    {"break", 0, true},
    {"continue", 0, true},

    {"br", 0, true},
    {"cond_br", 0, true},
    {"switch", 0, true},

    {"ret", 0, true},
    {"abort", BARRIER, true},
};

static_assert(sizeof(OPCODES) / sizeof(OPCODES[0]) == IR_OP_COUNT,
              "every opcode has its entry in the effects table");

const IrOpcodeInfo& haard::opcode_info(IrOpcode opcode) {
    return OPCODES[opcode];
}
