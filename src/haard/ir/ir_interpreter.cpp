#include <haard/ir/ir_interpreter.h>

#include <cmath>
#include <cstring>

using namespace haard;

// deep enough for any program the suites run, and a recursion that never
// ends stops with a message instead of taking the interpreter down with it
static const u32 DEEPEST = 10000;

static u64 pointer_to(u32 block, u32 offset) {
    return ((u64) block << 32) | offset;
}

static u32 block_of(u64 address) {
    return (u32) (address >> 32);
}

static u32 offset_of(u64 address) {
    return (u32) address;
}

static float as_f32(u64 bits) {
    u32 narrow = (u32) bits;
    float value;

    std::memcpy(&value, &narrow, sizeof(value));

    return value;
}

static double as_f64(u64 bits) {
    double value;

    std::memcpy(&value, &bits, sizeof(value));

    return value;
}

static u64 from_f32(float value) {
    u32 narrow;

    std::memcpy(&narrow, &value, sizeof(narrow));

    return narrow;
}

static u64 from_f64(double value) {
    u64 bits;

    std::memcpy(&bits, &value, sizeof(bits));

    return bits;
}

// the high 64 bits of a 128-bit product, from 32-bit halves: plain C++ with
// no 128-bit type, so the bootstrap can be written the same way
static u64 high_of_product(u64 a, u64 b) {
    u64 a_low = a & 0xffffffff;
    u64 a_high = a >> 32;
    u64 b_low = b & 0xffffffff;
    u64 b_high = b >> 32;
    u64 low_low = a_low * b_low;
    u64 high_low = a_high * b_low;
    u64 low_high = a_low * b_high;
    u64 high_high = a_high * b_high;
    u64 middle = (low_low >> 32) + (high_low & 0xffffffff)
               + (low_high & 0xffffffff);

    return high_high + (high_low >> 32) + (low_high >> 32) + (middle >> 32);
}

IrInterpreter::IrInterpreter() {
    program = nullptr;
    target = nullptr;
    output = nullptr;
    garbage = true;
    pattern = 0;
    depth = 0;
    stdout_handle = 0;
    stderr_handle = 0;
    stdin_handle = 0;
    at_function.index = 0;
    at_inst.index = 0;
    halted = false;
}

void IrInterpreter::set_program(IrProgram* program) {
    this->program = program;
}

void IrInterpreter::set_target(const Target* target) {
    this->target = target;
}

void IrInterpreter::set_output(std::ostream* output) {
    this->output = output;
}

void IrInterpreter::set_garbage(bool garbage) {
    this->garbage = garbage;
}

IrRunResult IrInterpreter::run(IrObjectId function,
                               const std::vector<u64>& arguments) {
    if (!prepare()) {
        return result;
    }

    return finish(function, arguments);
}

IrRunResult IrInterpreter::run_main(IrObjectId main,
                                    const std::vector<std::string>& arguments) {
    if (!prepare()) {
        return result;
    }

    const IrSignature& taken =
        program->get_signature(program->get_function(main).signature);

    if (taken.parameters.size() != 2) {
        return finish(main, {});
    }

    IrTypeId pointer = program->get_types()->pointer(IR_SPACE_GENERIC);
    u32 each = target->pointer_size;
    u32 table = allocate((u32) (arguments.size() + 1) * each, "argv", false);

    for (u32 i = 0; i < arguments.size(); i++) {
        u32 text = allocate((u32) arguments[i].size() + 1,
                            "argv[" + std::to_string(i) + "]", false);

        std::memcpy(memory[text].bytes.data(), arguments[i].data(),
                    arguments[i].size());
        store(pointer_to(table, i * each), pointer, pointer_to(text, 0));
    }

    return finish(main, {arguments.size(), pointer_to(table, 0)});
}

bool IrInterpreter::prepare() {
    halted = false;
    depth = 0;
    result = IrRunResult();
    result.kind = IR_RUN_EXIT;
    result.value = 0;
    result.function.index = 0;
    result.inst.index = 0;
    result.at.module = 0;
    result.at.offset = IR_NO_OFFSET;

    // the same pattern every run, so a golden that shows one is stable
    pattern = 0x9e3779b97f4a7c15ull;

    layouts = LayoutTable();
    layouts.set_target(target);
    layouts.set_types(program->get_types());

    if (!layouts.lay_out_all()) {
        stop(layouts.get_error());
        return false;
    }

    memory.assign(1, Memory());
    blocks_of.assign(program->get_object_count(), 0);

    // every object gets its block first, and only then are relocations
    // written, since a vtable may name a function placed after it
    for (u32 i = 1; i < program->get_object_count(); i++) {
        IrObject& object = program->get_object({i});

        if (object.kind == IR_OBJECT_FUNCTION) {
            blocks_of[i] = allocate(1, "@" + object.name, false);
        } else {
            u32 size = object.zeroed ? layouts.size_of(object.type)
                                     : (u32) object.bytes.size();

            blocks_of[i] = allocate(size, "@" + object.name, false);

            if (!object.zeroed) {
                memory[blocks_of[i]].bytes = object.bytes;
            }
        }

        memory[blocks_of[i]].object.index = i;
        memory[blocks_of[i]].read_only = object.read_only;
    }

    for (u32 i = 1; i < program->get_object_count(); i++) {
        IrObject& object = program->get_object({i});
        IrTypeId pointer = program->get_types()->pointer(IR_SPACE_GENERIC);

        for (const IrRelocation& one : object.relocations) {
            bool was = memory[blocks_of[i]].read_only;

            memory[blocks_of[i]].read_only = false;
            store(pointer_to(blocks_of[i], one.offset), pointer,
                  address_of(one.target));
            memory[blocks_of[i]].read_only = was;
        }
    }

    stdout_handle = pointer_to(allocate(1, "stdout", false), 0);
    stderr_handle = pointer_to(allocate(1, "stderr", false), 0);
    stdin_handle = pointer_to(allocate(1, "stdin", false), 0);

    return true;
}

IrRunResult IrInterpreter::finish(IrObjectId function,
                                  const std::vector<u64>& arguments) {
    for (IrObjectId one : program->get_initialisers()) {
        if (!halted) {
            call(one, {});
        }
    }

    u64 value = halted ? 0 : call(function, arguments);

    if (!halted) {
        result.value = value;
    }

    return result;
}

u64 IrInterpreter::call(IrObjectId id, const std::vector<u64>& arguments) {
    IrObject& object = program->get_object(id);

    if (object.native) {
        return native(id, arguments);
    }

    IrFunction& function = program->get_function(id);

    if (function.form != IR_FORM_CFG) {
        stop("@" + object.name + " is in the structured form, and only a CFG "
             "runs (question 78)");
        return 0;
    }

    if (++depth > DEEPEST) {
        stop("the calls are " + std::to_string(DEEPEST) + " deep");
        return 0;
    }

    Frame frame;
    IrBlockId block = function.body;
    IrBlockId none = {0};
    u64 given = 0;

    frame.function = id;
    frame.values.assign(function.values.size(), 0);

    for (u32 i = 0; i < arguments.size(); i++) {
        frame.values[function.parameter(i).index] = arguments[i];
    }

    enter(frame, function, none, block);

    bool left = false;

    while (!left && !halted) {
        IrBlockId next = {0};

        for (IrInstId id = function.get(block).first; id.index != 0;
             id = function.get(id).next) {
            given = execute(frame, function, id, next, left);

            if (halted || left || next.index != 0) {
                break;
            }
        }

        if (!left && !halted) {
            if (next.index == 0) {
                stop("ran off the end of ^" + std::to_string(block.index));
                break;
            }

            enter(frame, function, block, next);
            block = next;
        }
    }

    // a slot ends with its frame: a pointer to one kept past this is
    // stopped at its first use
    for (u32 slot : frame.slots) {
        memory[slot].live = false;
    }

    depth--;

    return given;
}

// a block's parameters were given by the 'br' that came here, and a loop's
// header counts the times its loop goes round
void IrInterpreter::enter(Frame& frame, IrFunction& function, IrBlockId from,
                          IrBlockId to) {
    IrBlock& block = function.get(to);

    if (!block.header) {
        return;
    }

    bool inside = false;

    for (IrBlockId at = from.index == 0 ? IrBlockId{0}
                                        : function.get(from).loop;
         at.index != 0; at = function.get(at).outer) {
        if (at.index == to.index) {
            inside = true;
        }
    }

    if (!inside) {
        frame.turns[to.index] = 0;
        return;
    }

    u32 turns = ++frame.turns[to.index];

    if (block.bound != IR_NO_BOUND && turns > block.bound) {
        stop("the loop at ^" + std::to_string(to.index)
             + " went round more than its bound of "
             + std::to_string(block.bound));
    }
}

u64 IrInterpreter::execute(Frame& frame, IrFunction& function, IrInstId id,
                           IrBlockId& next, bool& left) {
    IrInst& inst = function.get(id);
    IrOpcode opcode = (IrOpcode) inst.opcode;
    std::vector<u64>& values = frame.values;
    IrTypeTable* types = program->get_types();
    u64 made = 0;

    at_function = frame.function;
    at_inst = id;

    switch (opcode) {
    case IR_CONST:
        made = function.constants[inst.extra];
        break;

    case IR_NULL:
        made = 0;
        break;

    case IR_ADDR:
        made = address_of({inst.extra});
        break;

    case IR_SIZE:
        made = layouts.size_of(inst.type);
        break;

    case IR_ADD:
    case IR_SUB:
    case IR_MUL:
    case IR_SDIV:
    case IR_UDIV:
    case IR_SREM:
    case IR_UREM:
    case IR_SMULHI:
    case IR_UMULHI:
    case IR_AND:
    case IR_OR:
    case IR_XOR:
    case IR_SHL:
    case IR_LSHR:
    case IR_ASHR:
    case IR_FADD:
    case IR_FSUB:
    case IR_FMUL:
    case IR_FDIV:
        made = arithmetic(opcode, inst.type, values[inst.a], values[inst.b]);
        break;

    case IR_NEG:
        made = mask(0 - values[inst.a], bits(inst.type));
        break;

    case IR_NOT:
        made = mask(~values[inst.a], bits(inst.type));
        break;

    case IR_FNEG:
        made = types->get_type(inst.type)->kind == IR_F32
                   ? from_f32(-as_f32(values[inst.a]))
                   : from_f64(-as_f64(values[inst.a]));
        break;

    case IR_EQ:
    case IR_NE:
    case IR_SLT:
    case IR_SLE:
    case IR_SGT:
    case IR_SGE:
    case IR_ULT:
    case IR_ULE:
    case IR_UGT:
    case IR_UGE:
    case IR_FEQ:
    case IR_FNE:
    case IR_FLT:
    case IR_FLE:
    case IR_FGT:
    case IR_FGE:
        made = compare(opcode, inst.type, values[inst.a], values[inst.b]);
        break;

    case IR_TRUNC:
    case IR_ZEXT:
    case IR_SEXT:
    case IR_SITOFP:
    case IR_UITOFP:
    case IR_FPTOSI:
    case IR_FPTOUI:
    case IR_FPEXT:
    case IR_FPTRUNC:
    case IR_PTRTOINT:
    case IR_INTTOPTR:
        made = convert(opcode, function.values[inst.a].type, inst.type,
                       values[inst.a]);
        break;

    case IR_SELECT:
        made = values[inst.a] != 0 ? values[inst.b] : values[inst.c];
        break;

    case IR_SLOT: {
        u32 slot = allocate(layouts.size_of(inst.type), "a slot", garbage);

        frame.slots.push_back(slot);
        made = pointer_to(slot, 0);
        break;
    }

    case IR_LOAD:
    case IR_VOLATILE_LOAD:
        made = load(values[inst.a], inst.type);
        break;

    case IR_STORE:
    case IR_VOLATILE_STORE:
        store(values[inst.b], inst.type, values[inst.a]);
        break;

    case IR_COPY: {
        u32 size = layouts.size_of(inst.type);
        u8* from = reach(values[inst.b], size, false);
        u8* to = from == nullptr ? nullptr : reach(values[inst.a], size, true);

        if (to != nullptr) {
            std::memmove(to, from, size);
        }

        break;
    }

    case IR_FIELD:
        made = values[inst.a] + layouts.get_layout(inst.type)->offsets[inst.extra];
        break;

    case IR_CHECK_INDEX: {
        u32 width = bits(inst.type);

        if (mask(values[inst.a], width) >= mask(values[inst.b], width)) {
            abort("index " + std::to_string(widen(values[inst.a], width))
                  + " out of range for length "
                  + std::to_string(mask(values[inst.b], width)));
        }

        break;
    }

    // the index is read as signed, as a pointer may step back
    case IR_INDEX:
        made = values[inst.a]
             + (u64) (widen(values[inst.b], bits(function.values[inst.b].type))
                      * (i64) layouts.size_of(inst.type));
        break;

    case IR_UPCAST:
    case IR_DOWNCAST: {
        IrTypeId derived = opcode == IR_UPCAST ? inst.type : IrTypeId{inst.extra};
        IrTypeId base = opcode == IR_UPCAST ? IrTypeId{inst.extra} : inst.type;
        u64 offset = 0;

        for (IrTypeId at = derived; at.index != base.index;
             at = types->get_aggregate(at)->base) {
            offset += layouts.get_layout(at)->base_offset;
        }

        made = opcode == IR_UPCAST ? values[inst.a] + offset
                                   : values[inst.a] - offset;
        break;
    }

    case IR_CALL:
    case IR_CALL_INDIRECT:
    case IR_VCALL: {
        std::vector<u32> pool = function.pool_of(id);
        std::vector<u64> arguments;
        IrObjectId callee = {inst.extra};

        for (u32 one : pool) {
            arguments.push_back(values[one]);
        }

        if (opcode == IR_VCALL) {
            // what instruction selection will write: the vptr at the
            // receiver's offset 0, and the slot in the vtable it points to
            IrTypeId pointer = types->pointer(IR_SPACE_GENERIC);
            u64 table = load(arguments[0], pointer);
            u64 entry = halted ? 0
                               : load(table + (u64) inst.extra
                                                  * target->pointer_size,
                                      pointer);

            callee.index = halted ? 0 : memory[block_of(entry)].object.index;

            if (!halted && (offset_of(entry) != 0 || callee.index == 0
                            || program->get_object(callee).kind
                                   != IR_OBJECT_FUNCTION)) {
                stop("the vtable's slot " + std::to_string(inst.extra)
                     + " is not a function");
            }
        }

        if (opcode == IR_CALL_INDIRECT) {
            u32 block = block_of(values[inst.a]);

            callee.index = block < memory.size() ? memory[block].object.index
                                                 : 0;

            if (offset_of(values[inst.a]) != 0 || callee.index == 0
                || program->get_object(callee).kind != IR_OBJECT_FUNCTION) {
                stop("calls through something that is not a function");
            }
        }

        if (halted) {
            break;
        }

        made = call(callee, arguments);
        at_function = frame.function;
        at_inst = id;
        break;
    }

    case IR_FENCE:
        break;

    case IR_BR: {
        std::vector<u32> pool = function.pool_of(id);
        std::vector<u32> parameters = function.parameters_of({inst.extra});
        std::vector<u64> passed;

        // every argument read before any parameter is written, since a
        // block may pass its own parameters round again
        for (u32 one : pool) {
            passed.push_back(values[one]);
        }

        for (u32 i = 0; i < parameters.size(); i++) {
            values[parameters[i]] = passed[i];
        }

        next.index = inst.extra;
        break;
    }

    case IR_COND_BR:
        next.index = values[inst.a] != 0 ? inst.extra : inst.b;
        break;

    case IR_SWITCH: {
        std::vector<u32> pool = function.pool_of(id);

        next.index = inst.extra;

        for (u32 i = 0; i + 1 < pool.size(); i += 2) {
            if (mask(function.constants[pool[i]], bits(inst.type))
                == values[inst.a]) {
                next.index = pool[i + 1];
                break;
            }
        }

        break;
    }

    case IR_RET:
        left = true;
        return inst.a != 0 ? values[inst.a] : 0;

    case IR_ABORT: {
        IrObject& message = program->get_object({inst.extra});
        std::string text(message.bytes.begin(), message.bytes.end());

        abort(text.c_str());
        break;
    }

    default:
        stop(std::string("cannot run '") + opcode_info(opcode).name + "'");
        break;
    }

    if (inst.result.index != 0) {
        values[inst.result.index] = made;
    }

    return made;
}

u64 IrInterpreter::native(IrObjectId id, const std::vector<u64>& arguments) {
    const std::string& name = program->get_object(id).name;

    if (name == "__io_stdout") {
        return stdout_handle;
    }

    if (name == "__io_stderr") {
        return stderr_handle;
    }

    if (name == "__io_stdin") {
        return stdin_handle;
    }

    if (name == "__io_write" && arguments.size() == 2) {
        if (arguments[0] != stdout_handle && arguments[0] != stderr_handle) {
            stop("writes to a handle that is not stdout or stderr");
            return 0;
        }

        if (output != nullptr) {
            *output << (char) arguments[1];
        }

        return 0;
    }

    if (name == "__io_flush") {
        return 0;
    }

    if (name == "__abort") {
        abort("__abort");
        return 0;
    }

    // what is not written is the garbage pattern, as a slot's is
    if (name == "__alloc" && arguments.size() == 1) {
        if (arguments[0] > ((u64) 1 << 30)) {
            stop("an allocation of " + std::to_string(arguments[0])
                 + " bytes");
            return 0;
        }

        u32 made = allocate((u32) arguments[0],
                            "an allocation of "
                                + std::to_string(arguments[0]) + " bytes",
                            garbage);

        memory[made].heap = true;

        return pointer_to(made, 0);
    }

    // C++'s 'delete' of a null pointer does nothing, and so does this
    if (name == "__free" && arguments.size() == 1) {
        u32 block = block_of(arguments[0]);

        if (arguments[0] == 0) {
            return 0;
        }

        if (block >= memory.size() || !memory[block].heap
            || offset_of(arguments[0]) != 0) {
            stop("frees what no allocation gave");
            return 0;
        }

        if (!memory[block].live) {
            stop("frees " + memory[block].what + " a second time");
            return 0;
        }

        memory[block].live = false;

        return 0;
    }

    stop("the native @" + name + " is not in the interpreter yet");

    return 0;
}

u64 IrInterpreter::arithmetic(IrOpcode opcode, IrTypeId type, u64 left,
                              u64 right) {
    u32 width = bits(type);
    i64 a = widen(left, width);
    i64 b = widen(right, width);
    u64 top = width == 64 ? 0x8000000000000000ull : (u64) 1 << (width - 1);

    switch (opcode) {
    case IR_ADD:
        return mask(left + right, width);

    case IR_SUB:
        return mask(left - right, width);

    case IR_MUL:
        return mask(left * right, width);

    // question 3: a zero divisor aborts, and MIN / -1 is MIN, MIN % -1 is 0
    case IR_SDIV:
    case IR_SREM:
    case IR_UDIV:
    case IR_UREM:
        if (right == 0) {
            abort("division by zero");
            return 0;
        }

        if (opcode == IR_UDIV) {
            return left / right;
        }

        if (opcode == IR_UREM) {
            return left % right;
        }

        if (b == -1) {
            return opcode == IR_SDIV ? mask(0 - left, width) : 0;
        }

        return mask(opcode == IR_SDIV ? (u64) (a / b) : (u64) (a % b), width);

    case IR_UMULHI:
        return width == 64 ? high_of_product(left, right)
                           : mask((left * right) >> width, width);

    case IR_SMULHI:
        if (width == 64) {
            u64 high = high_of_product(left, right);

            high -= a < 0 ? right : 0;
            high -= b < 0 ? left : 0;

            return high;
        }

        return mask((u64) ((a * b) >> width), width);

    case IR_AND:
        return left & right;

    case IR_OR:
        return left | right;

    case IR_XOR:
        return left ^ right;

    // question 2: a count of the width or more gives the arithmetic result,
    // and a negative one is read as a large one, the count having no sign
    case IR_SHL:
        return right >= width ? 0 : mask(left << right, width);

    case IR_LSHR:
        return right >= width ? 0 : left >> right;

    case IR_ASHR:
        if (right >= width) {
            return (left & top) != 0 ? mask(~(u64) 0, width) : 0;
        }

        return mask((u64) (a >> right), width);

    case IR_FADD:
    case IR_FSUB:
    case IR_FMUL:
    case IR_FDIV:
        if (program->get_types()->get_type(type)->kind == IR_F32) {
            float x = as_f32(left);
            float y = as_f32(right);

            return from_f32(opcode == IR_FADD   ? x + y
                            : opcode == IR_FSUB ? x - y
                            : opcode == IR_FMUL ? x * y
                                                : x / y);
        } else {
            double x = as_f64(left);
            double y = as_f64(right);

            return from_f64(opcode == IR_FADD   ? x + y
                            : opcode == IR_FSUB ? x - y
                            : opcode == IR_FMUL ? x * y
                                                : x / y);
        }

    default:
        return 0;
    }
}

u64 IrInterpreter::compare(IrOpcode opcode, IrTypeId type, u64 left,
                           u64 right) {
    u32 width = bits(type);
    i64 a = widen(left, width);
    i64 b = widen(right, width);

    if (opcode >= IR_FEQ) {
        bool narrow = program->get_types()->get_type(type)->kind == IR_F32;
        double x = narrow ? as_f32(left) : as_f64(left);
        double y = narrow ? as_f32(right) : as_f64(right);

        // IEEE: every ordered comparison with a NaN is false, and so '!='
        // is true
        switch (opcode) {
        case IR_FEQ:
            return x == y;

        case IR_FNE:
            return x != y;

        case IR_FLT:
            return x < y;

        case IR_FLE:
            return x <= y;

        case IR_FGT:
            return x > y;

        default:
            return x >= y;
        }
    }

    switch (opcode) {
    case IR_EQ:
        return left == right;

    case IR_NE:
        return left != right;

    case IR_SLT:
        return a < b;

    case IR_SLE:
        return a <= b;

    case IR_SGT:
        return a > b;

    case IR_SGE:
        return a >= b;

    case IR_ULT:
        return left < right;

    case IR_ULE:
        return left <= right;

    case IR_UGT:
        return left > right;

    default:
        return left >= right;
    }
}

u64 IrInterpreter::convert(IrOpcode opcode, IrTypeId from, IrTypeId to,
                           u64 value) {
    u32 source = bits(from);
    u32 width = bits(to);
    bool narrow = program->get_types()->get_type(from)->kind == IR_F32;
    bool narrow_to = program->get_types()->get_type(to)->kind == IR_F32;

    switch (opcode) {
    case IR_TRUNC:
    case IR_ZEXT:
    case IR_PTRTOINT:
    case IR_INTTOPTR:
        return mask(value, width);

    case IR_SEXT:
        return mask((u64) widen(value, source), width);

    case IR_SITOFP:
        return narrow_to ? from_f32((float) widen(value, source))
                         : from_f64((double) widen(value, source));

    case IR_UITOFP:
        return narrow_to ? from_f32((float) value) : from_f64((double) value);

    case IR_FPTOSI:
    case IR_FPTOUI: {
        double real = narrow ? as_f32(value) : as_f64(value);
        double low = opcode == IR_FPTOSI ? -std::ldexp(1.0, width - 1) : 0.0;
        double high = opcode == IR_FPTOSI ? std::ldexp(1.0, width - 1)
                                          : std::ldexp(1.0, width);

        // Hadley, 2026-10-20: a float that does not fit the whole number, or
        // a NaN, aborts with a message, as a division by zero does (3a). It
        // fits when its whole part does: above one less than the lowest, and
        // below one more than the highest. A NaN is neither
        if (!(real > low - 1.0 && real < high)) {
            abort("a float that does not fit in the whole number");
            return 0;
        }

        return opcode == IR_FPTOSI ? mask((u64) (i64) real, width)
                                   : mask((u64) real, width);
    }

    case IR_FPEXT:
        return from_f64((double) as_f32(value));

    case IR_FPTRUNC:
        return from_f32((float) as_f64(value));

    default:
        return value;
    }
}

u32 IrInterpreter::allocate(u32 size, const std::string& what,
                            bool fill) {
    Memory made;

    made.bytes.assign(size, 0);
    made.live = true;
    made.heap = false;
    made.read_only = false;
    made.object.index = 0;
    made.what = what;

    // question 10: memory nobody wrote holds some pattern, the same one on
    // every run here, and a program must not depend on it
    if (fill) {
        for (u32 i = 0; i < size; i++) {
            pattern ^= pattern << 13;
            pattern ^= pattern >> 7;
            pattern ^= pattern << 17;
            made.bytes[i] = (u8) pattern;
        }
    }

    memory.push_back(made);

    return (u32) memory.size() - 1;
}

u8* IrInterpreter::reach(u64 address, u32 size, bool write) {
    u32 block = block_of(address);
    u32 offset = offset_of(address);

    if (block == 0) {
        stop("an access through a null pointer");
        return nullptr;
    }

    if (block >= memory.size()) {
        stop("an access through a pointer to nothing");
        return nullptr;
    }

    Memory& held = memory[block];

    if (!held.live && held.heap) {
        stop("an access to " + held.what + " after it was freed");
        return nullptr;
    }

    if (!held.live) {
        stop("an access to " + held.what + " of a function that returned");
        return nullptr;
    }

    if ((u64) offset + size > held.bytes.size()) {
        stop("an access of " + std::to_string(size) + " bytes at "
             + std::to_string(offset) + " in " + held.what + ", which has "
             + std::to_string(held.bytes.size()));
        return nullptr;
    }

    if (write && held.read_only) {
        stop("a write to " + held.what + ", which is read-only");
        return nullptr;
    }

    return held.bytes.data() + offset;
}

u64 IrInterpreter::load(u64 address, IrTypeId type) {
    u32 size = layouts.size_of(type);
    u8* bytes = reach(address, size, false);
    u64 value = 0;

    if (bytes == nullptr) {
        return 0;
    }

    for (u32 i = 0; i < size; i++) {
        u32 at = target->big_endian ? i : size - 1 - i;

        value = (value << 8) | bytes[at];
    }

    return value;
}

void IrInterpreter::store(u64 address, IrTypeId type, u64 value) {
    u32 size = layouts.size_of(type);
    u8* bytes = reach(address, size, true);

    if (bytes == nullptr) {
        return;
    }

    for (u32 i = 0; i < size; i++) {
        u32 at = target->big_endian ? size - 1 - i : i;

        bytes[at] = (u8) (value >> (8 * i));
    }
}

u64 IrInterpreter::address_of(IrObjectId object) {
    return pointer_to(blocks_of[object.index], 0);
}

u32 IrInterpreter::bits(IrTypeId type) {
    switch (program->get_types()->get_type(type)->kind) {
    case IR_BOOL:
        return 1;

    case IR_B8:
        return 8;

    case IR_B16:
        return 16;

    case IR_B32:
    case IR_F32:
        return 32;

    default:
        return 64;
    }
}

u64 IrInterpreter::mask(u64 value, u32 width) {
    return width >= 64 ? value : value & (((u64) 1 << width) - 1);
}

i64 IrInterpreter::widen(u64 value, u32 width) {
    if (width >= 64) {
        return (i64) value;
    }

    u64 top = (u64) 1 << (width - 1);

    return (i64) ((mask(value, width) ^ top) - top);
}

void IrInterpreter::stop(const std::string& message) {
    if (halted) {
        return;
    }

    halted = true;
    result.kind = IR_RUN_STOP;
    result.message = message;
    result.function = at_function;
    result.inst = at_inst;

    if (at_function.index != 0 && at_inst.index != 0) {
        result.at = program->get_function(at_function).positions[at_inst.index];
    }
}

void IrInterpreter::abort(const std::string& message) {
    stop(message);
    result.kind = IR_RUN_ABORT;
}
