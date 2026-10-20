#include <haard/ir/ir_builder.h>

using namespace haard;

static const IrTypeId NO_TYPE = {0};

static std::vector<u32> no_pool() {
    return std::vector<u32>();
}

static std::vector<u32> values_of(const std::vector<IrValueId>& values) {
    std::vector<u32> out;

    for (IrValueId one : values) {
        out.push_back(one.index);
    }

    return out;
}

IrBuilder::IrBuilder() {
    program = nullptr;
    current.index = 0;
    block.index = 0;
}

void IrBuilder::set_program(IrProgram* program) {
    this->program = program;
}

void IrBuilder::begin_function(IrObjectId object) {
    IrInstId none = {0};

    current = object;
    open.clear();

    const IrSignature& signature =
        program->get_signature(function().signature);

    for (u32 i = 0; i < signature.parameters.size(); i++) {
        add_value(IR_VALUE_PARAMETER, signature.parameters[i], i);
    }

    block = add_block(none);
    function().body = block;

    if (function().form == IR_FORM_CFG) {
        function().order.push_back(block);
    }
}

IrValueId IrBuilder::parameter(u32 which) {
    return function().parameter(which);
}

IrValueId IrBuilder::constant(IrTypeId type, u64 bits, IrPosition at) {
    u32 index = (u32) function().constants.size();

    function().constants.push_back(bits);

    return result_of(emit(IR_CONST, type, 0, 0, 0, index, no_pool(), type, at));
}

IrValueId IrBuilder::null(IrPosition at) {
    return result_of(
        emit(IR_NULL, pointer(), 0, 0, 0, 0, no_pool(), pointer(), at));
}

IrValueId IrBuilder::address(IrObjectId object, IrPosition at) {
    return result_of(emit(IR_ADDR, pointer(), 0, 0, 0, object.index,
                          no_pool(), pointer(), at));
}

IrValueId IrBuilder::size(IrTypeId type, IrTypeId result, IrPosition at) {
    return result_of(emit(IR_SIZE, type, 0, 0, 0, 0, no_pool(), result, at));
}

IrValueId IrBuilder::binary(IrOpcode opcode, IrTypeId type, IrValueId left,
                            IrValueId right, IrPosition at) {
    IrTypeId result = opcode >= IR_EQ && opcode <= IR_FGE
                          ? program->get_types()->scalar(IR_BOOL)
                          : type;

    return result_of(emit(opcode, type, left.index, right.index, 0, 0,
                          no_pool(), result, at));
}

IrValueId IrBuilder::unary(IrOpcode opcode, IrTypeId type, IrValueId operand,
                           IrPosition at) {
    return result_of(
        emit(opcode, type, operand.index, 0, 0, 0, no_pool(), type, at));
}

IrValueId IrBuilder::convert(IrOpcode opcode, IrTypeId to, IrValueId operand,
                             IrPosition at) {
    return result_of(
        emit(opcode, to, operand.index, 0, 0, 0, no_pool(), to, at));
}

IrValueId IrBuilder::select(IrTypeId type, IrValueId condition,
                            IrValueId chosen, IrValueId otherwise,
                            IrPosition at) {
    return result_of(emit(IR_SELECT, type, condition.index, chosen.index,
                          otherwise.index, 0, no_pool(), type, at));
}

IrValueId IrBuilder::slot(IrTypeId type, IrPosition at) {
    return result_of(
        emit(IR_SLOT, type, 0, 0, 0, 0, no_pool(), pointer(), at));
}

IrValueId IrBuilder::load(IrTypeId type, IrValueId from, IrPosition at) {
    return result_of(
        emit(IR_LOAD, type, from.index, 0, 0, 0, no_pool(), type, at));
}

void IrBuilder::store(IrTypeId type, IrValueId value, IrValueId to,
                      IrPosition at) {
    emit(IR_STORE, type, value.index, to.index, 0, 0, no_pool(), NO_TYPE, at);
}

void IrBuilder::copy(IrTypeId type, IrValueId to, IrValueId from,
                     IrPosition at) {
    emit(IR_COPY, type, to.index, from.index, 0, 0, no_pool(), NO_TYPE, at);
}

IrValueId IrBuilder::field(IrTypeId aggregate, IrValueId of, u32 which,
                           IrPosition at) {
    return result_of(emit(IR_FIELD, aggregate, of.index, 0, 0, which,
                          no_pool(), pointer(), at));
}

IrValueId IrBuilder::index(IrTypeId element, IrValueId of, IrValueId which,
                           IrPosition at) {
    return result_of(emit(IR_INDEX, element, of.index, which.index, 0, 0,
                          no_pool(), pointer(), at));
}

IrValueId IrBuilder::upcast(IrTypeId from, IrTypeId to, IrValueId of,
                            IrPosition at) {
    return result_of(emit(IR_UPCAST, from, of.index, 0, 0, to.index,
                          no_pool(), pointer(), at));
}

IrValueId IrBuilder::downcast(IrTypeId from, IrTypeId to, IrValueId of,
                              IrPosition at) {
    return result_of(emit(IR_DOWNCAST, from, of.index, 0, 0, to.index,
                          no_pool(), pointer(), at));
}

IrValueId IrBuilder::call(IrObjectId target,
                          const std::vector<IrValueId>& arguments,
                          IrPosition at) {
    IrObject& object = program->get_object(target);
    u32 signature = object.kind == IR_OBJECT_FUNCTION
                        ? program->get_function(target).signature
                        : 0;
    IrTypeId result = program->get_signature(signature).result;

    return result_of(emit(IR_CALL, NO_TYPE, 0, 0, 0, target.index,
                          values_of(arguments), result, at));
}

IrValueId IrBuilder::call_indirect(u32 signature, IrValueId target,
                                   const std::vector<IrValueId>& arguments,
                                   IrPosition at) {
    IrTypeId result = program->get_signature(signature).result;

    return result_of(emit(IR_CALL_INDIRECT, NO_TYPE, target.index, 0, 0,
                          signature, values_of(arguments), result, at));
}

IrValueId IrBuilder::vcall(IrTypeId type, u32 slot, u32 signature,
                           const std::vector<IrValueId>& arguments,
                           IrPosition at) {
    IrTypeId result = program->get_signature(signature).result;

    // the signature is the slot's, kept in 'c' so 'extra' names the slot
    return result_of(emit(IR_VCALL, type, 0, 0, signature, slot,
                          values_of(arguments), result, at));
}

IrValueId IrBuilder::volatile_load(IrTypeId type, IrValueId from,
                                   IrPosition at) {
    return result_of(emit(IR_VOLATILE_LOAD, type, from.index, 0, 0, 0,
                          no_pool(), type, at));
}

void IrBuilder::volatile_store(IrTypeId type, IrValueId value, IrValueId to,
                               IrPosition at) {
    emit(IR_VOLATILE_STORE, type, value.index, to.index, 0, 0, no_pool(),
         NO_TYPE, at);
}

void IrBuilder::fence(IrPosition at) {
    emit(IR_FENCE, NO_TYPE, 0, 0, 0, 0, no_pool(), NO_TYPE, at);
}

IrInstId IrBuilder::begin_block(IrPosition at) {
    IrInstId region = emit(IR_BLOCK, NO_TYPE, 0, 0, 0, 0, no_pool(), NO_TYPE,
                           at);
    IrBlockId body = add_block(region);

    function().get(region).extra = body.index;
    open.push_back(std::make_pair(region, block));
    block = body;

    return region;
}

IrInstId IrBuilder::begin_if(IrValueId condition, IrPosition at) {
    IrInstId region = emit(IR_IF, NO_TYPE, condition.index, 0, 0, 0,
                           no_pool(), NO_TYPE, at);
    IrBlockId body = add_block(region);

    function().get(region).extra = body.index;
    open.push_back(std::make_pair(region, block));
    block = body;

    return region;
}

void IrBuilder::begin_else() {
    IrInstId region = open.back().first;
    IrBlockId otherwise = add_block(region);

    function().get(region).c = otherwise.index;
    block = otherwise;
}

IrInstId IrBuilder::begin_loop(u32 bound, IrPosition at) {
    IrInstId region = emit(IR_LOOP, NO_TYPE, 0, 0, bound, 0, no_pool(),
                           NO_TYPE, at);
    IrBlockId body = add_block(region);

    function().get(region).extra = body.index;
    open.push_back(std::make_pair(region, block));
    block = body;

    return region;
}

void IrBuilder::end() {
    block = open.back().second;
    open.pop_back();
}

void IrBuilder::break_to(IrInstId region, IrPosition at) {
    emit(IR_BREAK, NO_TYPE, 0, 0, 0, region.index, no_pool(), NO_TYPE, at);
}

void IrBuilder::continue_to(IrInstId region, IrPosition at) {
    emit(IR_CONTINUE, NO_TYPE, 0, 0, 0, region.index, no_pool(), NO_TYPE, at);
}

IrBlockId IrBuilder::new_block(const std::vector<IrTypeId>& parameters) {
    IrInstId none = {0};
    IrBlockId made = add_block(none);
    std::vector<u32> values;

    for (IrTypeId type : parameters) {
        values.push_back(
            add_value(IR_VALUE_BLOCK_PARAMETER, type, made.index).index);
    }

    IrFunction& body = function();

    body.get(made).pool_start = (u32) body.pool.size();
    body.get(made).pool_count = (u32) values.size();
    body.pool.insert(body.pool.end(), values.begin(), values.end());
    body.order.push_back(made);

    return made;
}

IrValueId IrBuilder::block_parameter(IrBlockId target, u32 which) {
    IrValueId value = {function().parameters_of(target)[which]};

    return value;
}

void IrBuilder::set_block(IrBlockId target) {
    block = target;
}

void IrBuilder::br(IrBlockId target, const std::vector<IrValueId>& arguments,
                   IrPosition at) {
    emit(IR_BR, NO_TYPE, 0, 0, 0, target.index, values_of(arguments), NO_TYPE,
         at);
}

void IrBuilder::cond_br(IrValueId condition, IrBlockId chosen,
                        IrBlockId otherwise, IrPosition at) {
    emit(IR_COND_BR, NO_TYPE, condition.index, otherwise.index, 0,
         chosen.index, no_pool(), NO_TYPE, at);
}

// the pool holds each case as two entries, the constant's index and the block
void IrBuilder::switch_on(IrTypeId type, IrValueId value, IrBlockId otherwise,
                          const std::vector<std::pair<u64, IrBlockId>>& cases,
                          IrPosition at) {
    std::vector<u32> pool;

    for (const std::pair<u64, IrBlockId>& one : cases) {
        pool.push_back((u32) function().constants.size());
        pool.push_back(one.second.index);
        function().constants.push_back(one.first);
    }

    emit(IR_SWITCH, type, value.index, 0, 0, otherwise.index, pool, NO_TYPE,
         at);
}

void IrBuilder::ret(IrPosition at) {
    emit(IR_RET, NO_TYPE, 0, 0, 0, 0, no_pool(), NO_TYPE, at);
}

void IrBuilder::ret(IrTypeId type, IrValueId value, IrPosition at) {
    emit(IR_RET, type, value.index, 0, 0, 0, no_pool(), NO_TYPE, at);
}

void IrBuilder::abort(IrObjectId message, IrPosition at) {
    emit(IR_ABORT, NO_TYPE, 0, 0, 0, message.index, no_pool(), NO_TYPE, at);
}

IrInstId IrBuilder::emit(IrOpcode opcode, IrTypeId type, u32 a, u32 b, u32 c,
                         u32 extra, const std::vector<u32>& pool,
                         IrTypeId result, IrPosition at) {
    IrFunction& body = function();
    IrInst inst = {};
    IrInstId made = {(u32) body.instructions.size()};

    inst.opcode = opcode;
    inst.type = type;
    inst.a = a;
    inst.b = b;
    inst.c = c;
    inst.extra = extra;
    inst.pool_start = (u32) body.pool.size();
    inst.pool_count = (u32) pool.size();
    inst.block = block;
    inst.previous = body.get(block).last;
    body.pool.insert(body.pool.end(), pool.begin(), pool.end());

    if (result.index != 0) {
        inst.result = add_value(IR_VALUE_RESULT, result, made.index);
    }

    body.instructions.push_back(inst);
    body.positions.push_back(at);

    if (body.get(block).last.index != 0) {
        body.get(body.get(block).last).next = made;
    } else {
        body.get(block).first = made;
    }

    body.get(block).last = made;

    return made;
}

IrBlockId IrBuilder::add_block(IrInstId region) {
    IrFunction& body = function();
    IrBlock made = {};
    IrBlockId id = {(u32) body.blocks.size()};

    made.region = region;
    body.blocks.push_back(made);

    return id;
}

IrValueId IrBuilder::add_value(IrValueKind kind, IrTypeId type, u32 owner) {
    IrFunction& body = function();
    IrValue made;
    IrValueId id = {(u32) body.values.size()};

    made.kind = kind;
    made.type = type;
    made.owner = owner;
    body.values.push_back(made);

    return id;
}

IrValueId IrBuilder::result_of(IrInstId inst) {
    return function().get(inst).result;
}

IrTypeId IrBuilder::pointer() {
    return program->get_types()->pointer(IR_SPACE_GENERIC);
}

IrFunction& IrBuilder::function() {
    return program->get_function(current);
}
