#include <haard/ir/ir_flattener.h>

using namespace haard;

IrFlattener::IrFlattener() {
    program = nullptr;
    current.index = 0;
    loop.index = 0;
}

void IrFlattener::set_program(IrProgram* program) {
    this->program = program;
}

void IrFlattener::flatten(IrObjectId function) {
    in = program->get_function(function);

    if (in.form == IR_FORM_CFG) {
        return;
    }

    out = IrFunction();
    out.object = in.object;
    out.signature = in.signature;
    out.form = IR_FORM_CFG;
    out.real_time = in.real_time;

    // values keep their numbers, and a copied instruction's pooled operands
    // keep their place in the pool
    out.values = in.values;
    out.constants = in.constants;
    out.pool = in.pool;

    frames.clear();
    jumps_to.assign(1, 0);
    loop.index = 0;
    current.index = 0;

    out.body = make_block();
    place(out.body);
    flatten_block(in.body);

    // the last block too, which is where an 'if' that returns on both sides
    // at the end of a function leaves its empty block
    close();
    program->get_function(function) = out;
}

void IrFlattener::flatten_block(IrBlockId block) {
    for (IrInstId id = in.get(block).first; id.index != 0;
         id = in.get(id).next) {
        IrInst& inst = in.get(id);
        IrPosition at = in.positions[id.index];

        switch (inst.opcode) {
        case IR_BLOCK: {
            Frame frame = {id.index, make_block(), IrBlockId{0}};

            frames.push_back(frame);
            flatten_block({inst.extra});

            if (!ended()) {
                jump(frame.after, at);
            }

            frames.pop_back();
            place(frame.after);
            break;
        }

        case IR_IF: {
            IrBlockId chosen = make_block();
            IrBlockId otherwise = inst.c != 0 ? make_block() : IrBlockId{0};
            Frame frame = {id.index, make_block(), IrBlockId{0}};

            branch(inst.a, chosen,
                   otherwise.index != 0 ? otherwise : frame.after, at);
            frames.push_back(frame);
            place(chosen);
            flatten_block({inst.extra});

            if (!ended()) {
                jump(frame.after, at);
            }

            if (otherwise.index != 0) {
                place(otherwise);
                flatten_block({inst.c});

                if (!ended()) {
                    jump(frame.after, at);
                }
            }

            frames.pop_back();
            place(frame.after);
            break;
        }

        case IR_LOOP: {
            IrBlockId around = loop;
            IrBlockId top = make_block();
            Frame frame = {id.index, make_block(), top};

            out.get(top).header = true;
            out.get(top).bound = inst.c;
            out.get(top).loop = top;
            out.get(top).outer = around;
            jump(top, at);
            frames.push_back(frame);
            loop = top;
            place(top);
            flatten_block({inst.extra});

            if (!ended()) {
                jump(top, at);
            }

            loop = around;
            frames.pop_back();
            place(frame.after);
            break;
        }

        case IR_BREAK:
            jump(frame_of(inst.extra).after, at);
            break;

        case IR_CONTINUE:
            jump(frame_of(inst.extra).top, at);
            break;

        default:
            copy(id);
            break;
        }
    }
}

void IrFlattener::copy(IrInstId id) {
    IrInstId made = append(in.get(id), in.positions[id.index]);
    IrInst& inst = out.get(made);

    if (inst.result.index != 0) {
        out.get(inst.result).owner = made.index;
    }
}

void IrFlattener::jump(IrBlockId target, IrPosition at) {
    IrInst inst = {};

    inst.opcode = IR_BR;
    inst.extra = target.index;
    jumps_to[target.index]++;
    append(inst, at);
}

void IrFlattener::branch(u32 condition, IrBlockId chosen, IrBlockId otherwise,
                         IrPosition at) {
    IrInst inst = {};

    inst.opcode = IR_COND_BR;
    inst.a = condition;
    inst.b = otherwise.index;
    inst.extra = chosen.index;
    jumps_to[chosen.index]++;
    jumps_to[otherwise.index]++;
    append(inst, at);
}

IrInstId IrFlattener::append(const IrInst& source, IrPosition at) {
    IrInst inst = source;
    IrInstId made = {(u32) out.instructions.size()};
    IrBlock& block = out.get(current);

    inst.block = current;
    inst.previous = block.last;
    inst.next.index = 0;

    if (block.last.index != 0) {
        out.get(block.last).next = made;
    } else {
        block.first = made;
    }

    block.last = made;
    out.instructions.push_back(inst);
    out.positions.push_back(at);

    return made;
}

IrBlockId IrFlattener::make_block() {
    IrBlock made = {};
    IrBlockId id = {(u32) out.blocks.size()};

    made.bound = IR_NO_BOUND;
    made.loop = loop;
    out.blocks.push_back(made);
    jumps_to.push_back(0);

    return id;
}

// The block that was being written is closed. An empty one nothing jumps to
// leaves the order: an 'if' whose two sides both left has nothing after it
void IrFlattener::close() {
    if (current.index != 0 && out.order.size() > 1
        && out.get(current).first.index == 0
        && jumps_to[current.index] == 0) {
        out.order.pop_back();
    }
}

void IrFlattener::place(IrBlockId block) {
    close();
    out.get(block).loop = out.get(block).header ? block : loop;
    out.order.push_back(block);
    current = block;
}

bool IrFlattener::ended() {
    IrInstId last = out.get(current).last;

    return last.index != 0
        && opcode_info((IrOpcode) out.get(last).opcode).terminator;
}

const IrFlattener::Frame& IrFlattener::frame_of(u32 region) {
    for (u32 i = (u32) frames.size(); i > 0; i--) {
        if (frames[i - 1].region == region) {
            return frames[i - 1];
        }
    }

    // the verifier has made sure every 'break' and 'continue' names a region
    // around it, and flatten runs only on what it passed
    return frames.back();
}
