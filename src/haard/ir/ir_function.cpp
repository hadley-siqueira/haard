#include <haard/ir/ir_function.h>

using namespace haard;

IrFunction::IrFunction() {
    IrInst nothing = {};
    IrBlock no_block = {};
    IrValue no_value = {};
    IrPosition nowhere = {0, IR_NO_OFFSET};

    object.index = 0;
    signature = 0;
    form = IR_FORM_STRUCTURED;
    real_time = false;
    body.index = 0;

    // index 0 of each is 'none'
    instructions.push_back(nothing);
    blocks.push_back(no_block);
    values.push_back(no_value);
    positions.push_back(nowhere);
}

IrInst& IrFunction::get(IrInstId id) {
    return instructions[id.index];
}

IrBlock& IrFunction::get(IrBlockId id) {
    return blocks[id.index];
}

IrValue& IrFunction::get(IrValueId id) {
    return values[id.index];
}

// the parameters are the first values, in order, which is how they were made
IrValueId IrFunction::parameter(u32 which) {
    IrValueId id = {which + 1};

    return id;
}

std::vector<u32> IrFunction::pool_of(IrInstId id) {
    IrInst& inst = instructions[id.index];

    return std::vector<u32>(pool.begin() + inst.pool_start,
                            pool.begin() + inst.pool_start + inst.pool_count);
}

std::vector<u32> IrFunction::parameters_of(IrBlockId id) {
    IrBlock& block = blocks[id.index];

    return std::vector<u32>(pool.begin() + block.pool_start,
                            pool.begin() + block.pool_start + block.pool_count);
}
