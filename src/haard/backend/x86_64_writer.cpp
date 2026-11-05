#include <haard/backend/x86_64_writer.h>

using namespace haard;

// in the order record 0084 gives them
static const char* const ARGUMENTS[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
static const u32 ARGUMENTS_IN_REGISTERS = 6;

X8664Writer::X8664Writer() {
    program = nullptr;
    target = nullptr;
    current = 0;
    frame_size = 0;
    fresh = 0;
}

void X8664Writer::set_program(IrProgram* program) {
    this->program = program;
}

void X8664Writer::set_target(const Target* target) {
    this->target = target;
}

const std::string& X8664Writer::get_error() {
    return error;
}

bool X8664Writer::refuse(const std::string& what) {
    error = "the native back end does not write " + what + " yet";

    return false;
}

void X8664Writer::line(const std::string& written) {
    text << "    " << written << '\n';
}

std::string X8664Writer::quoted(const std::string& name) {
    return '"' + name + '"';
}

bool X8664Writer::write(std::ostream& out) {
    layouts.set_target(target);
    layouts.set_types(program->get_types());

    if (!layouts.lay_out_all()) {
        error = layouts.get_error();

        return false;
    }

    for (u32 i = 1; i < program->get_object_count(); i++) {
        IrObject& object = program->get_object({i});

        if (object.kind == IR_OBJECT_FUNCTION && !object.native
            && !write_function({i})) {
            return false;
        }
    }

    write_order("__before_main", program->get_initialisers(), false);
    write_order("__after_main", program->get_finalisers(), true);

    // what every division in the program stops at
    text << ".Ldivision_by_zero:\n";
    line("lea rdi, [rip + .Lmessage_division_by_zero]");
    line("call __abort_with");
    line("ud2");
    text << ".section .rodata\n";
    text << ".Lmessage_division_by_zero:\n";
    line(".asciz \"division by zero\"");

    out << text.str();

    return true;
}

// the IR's initialisers in the order they were added, its finalisers the last
// first, which are the two functions the runtime's _start calls
void X8664Writer::write_order(const char* label,
                              const std::vector<IrObjectId>& functions,
                              bool backwards) {
    text << label << ":\n";

    for (u32 i = 0; i < functions.size(); i++) {
        u32 at = backwards ? (u32) functions.size() - 1 - i : i;

        line("call " + quoted(program->get_object(functions[at]).name));
    }

    line("ret");
}

// every value its eight bytes, then every slot where its type asks. All of it
// is fixed when the function starts (question 13)
bool X8664Writer::lay_out_frame(IrFunction& function) {
    u32 size = (u32) function.values.size() * 8;

    slot_offsets.clear();

    for (IrBlockId block : function.order) {
        for (IrInstId id = function.get(block).first; id.index != 0;
             id = function.get(id).next) {
            const IrInst& inst = function.get(id);

            if (inst.opcode == IR_SLOT) {
                u32 align = layouts.align_of(inst.type);

                size += layouts.size_of(inst.type);
                size = (size + align - 1) / align * align;
                slot_offsets[id.index] = size;
            }
        }
    }

    frame_size = (size + 15) / 16 * 16;

    return true;
}

bool X8664Writer::write_function(IrObjectId object) {
    IrFunction& function = program->get_function(object);
    const IrSignature& signature = program->get_signature(function.signature);

    current = object.index;
    lay_out_frame(function);

    text << quoted(program->get_object(object).name) << ":\n";
    line("push rbp");
    line("mov rbp, rsp");
    line("sub rsp, " + std::to_string(frame_size));

    for (u32 i = 0; i < signature.parameters.size(); i++) {
        if (is_float(signature.parameters[i])) {
            return refuse("a float as a parameter");
        }

        u32 value = function.parameter(i).index;

        if (i < ARGUMENTS_IN_REGISTERS) {
            line("mov qword ptr " + place(value) + ", "
                 + ARGUMENTS[i]);
        } else {
            // above the saved rbp and the return address
            line("mov rax, qword ptr [rbp + "
                 + std::to_string(16 + (i - ARGUMENTS_IN_REGISTERS) * 8) + "]");
            line("mov qword ptr " + place(value) + ", rax");
        }
    }

    for (IrBlockId block : function.order) {
        text << block_label(block) << ":\n";

        for (IrInstId id = function.get(block).first; id.index != 0;
             id = function.get(id).next) {
            if (!write_instruction(function, id)) {
                return false;
            }
        }
    }

    return true;
}

std::string X8664Writer::place(u32 value) {
    return "[rbp - " + std::to_string(value * 8) + "]";
}

void X8664Writer::load(const char* reg, u32 value) {
    line(std::string("mov ") + reg + ", qword ptr " + place(value));
}

void X8664Writer::store_result(const IrInst& inst) {
    if (inst.result.index != 0) {
        line("mov qword ptr " + place(inst.result.index) + ", rax");
    }
}

// what the interpreter's 'bits' says: a bool is one bit
u32 X8664Writer::width_of(IrTypeId type) {
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

bool X8664Writer::is_float(IrTypeId type) {
    u8 kind = program->get_types()->get_type(type)->kind;

    return kind == IR_F32 || kind == IR_F64;
}

void X8664Writer::mask(u32 width) {
    switch (width) {
    case 1:
        line("and eax, 1");
        break;

    case 8:
        line("movzx eax, al");
        break;

    case 16:
        line("movzx eax, ax");
        break;

    case 32:
        line("mov eax, eax");
        break;

    default:
        break;
    }
}

void X8664Writer::widen(const char* wide, u32 width) {
    bool first = std::string(wide) == "rax";

    switch (width) {
    case 8:
        line(std::string("movsx ") + wide + (first ? ", al" : ", cl"));
        break;

    case 16:
        line(std::string("movsx ") + wide + (first ? ", ax" : ", cx"));
        break;

    case 32:
        line(std::string("movsxd ") + wide + (first ? ", eax" : ", ecx"));
        break;

    default:
        break;
    }
}

std::string X8664Writer::block_label(IrBlockId block) {
    return ".L" + std::to_string(current) + "_" + std::to_string(block.index);
}

std::string X8664Writer::fresh_label() {
    fresh++;

    return ".Lt" + std::to_string(fresh);
}

bool X8664Writer::write_instruction(IrFunction& function, IrInstId id) {
    const IrInst& inst = function.get(id);
    IrOpcode opcode = (IrOpcode) inst.opcode;

    switch (opcode) {
    // a constant is held as a value is, zero extended from its width
    case IR_CONST:
        line("mov rax, " + std::to_string(function.constants[inst.extra]));
        store_result(inst);

        return true;

    case IR_NULL:
        line("xor eax, eax");
        store_result(inst);

        return true;

    case IR_ADD:
    case IR_SUB:
    case IR_MUL:
    case IR_SDIV:
    case IR_UDIV:
    case IR_SREM:
    case IR_UREM:
    case IR_AND:
    case IR_OR:
    case IR_XOR:
    case IR_SHL:
    case IR_LSHR:
    case IR_ASHR:
        return write_binary(inst);

    case IR_NEG:
        load("rax", inst.a);
        line("neg rax");
        mask(width_of(inst.type));
        store_result(inst);

        return true;

    case IR_NOT:
        load("rax", inst.a);
        line("not rax");
        mask(width_of(inst.type));
        store_result(inst);

        return true;

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
        write_comparison(inst, width_of(inst.type));

        return true;

    case IR_TRUNC:
    case IR_ZEXT:
        load("rax", inst.a);
        mask(width_of(inst.type));
        store_result(inst);

        return true;

    case IR_SEXT:
        load("rax", inst.a);
        widen("rax", width_of(function.get(IrValueId{inst.a}).type));
        mask(width_of(inst.type));
        store_result(inst);

        return true;

    case IR_SELECT:
        load("rax", inst.c);
        load("rcx", inst.b);
        load("rdx", inst.a);
        line("test rdx, rdx");
        line("cmovne rax, rcx");
        store_result(inst);

        return true;

    case IR_SLOT:
        line("lea rax, [rbp - " + std::to_string(slot_offsets[id.index]) + "]");
        store_result(inst);

        return true;

    case IR_LOAD:
        load("rcx", inst.a);

        switch (width_of(inst.type)) {
        case 1:
        case 8:
            line("movzx eax, byte ptr [rcx]");
            break;

        case 16:
            line("movzx eax, word ptr [rcx]");
            break;

        case 32:
            line("mov eax, dword ptr [rcx]");
            break;

        default:
            line("mov rax, qword ptr [rcx]");
            break;
        }

        store_result(inst);

        return true;

    case IR_STORE:
        load("rax", inst.a);
        load("rcx", inst.b);

        switch (width_of(inst.type)) {
        case 1:
        case 8:
            line("mov byte ptr [rcx], al");
            break;

        case 16:
            line("mov word ptr [rcx], ax");
            break;

        case 32:
            line("mov dword ptr [rcx], eax");
            break;

        default:
            line("mov qword ptr [rcx], rax");
            break;
        }

        return true;

    case IR_CALL:
        return write_call(function, id);

    case IR_BR:
        if (function.get(id).pool_count > 0) {
            return refuse("a block that takes arguments");
        }

        line("jmp " + block_label({inst.extra}));

        return true;

    case IR_COND_BR:
        load("rax", inst.a);
        line("test rax, rax");
        line("jne " + block_label({inst.extra}));
        line("jmp " + block_label({inst.b}));

        return true;

    case IR_RET:
        if (inst.a != 0) {
            if (is_float(inst.type)) {
                return refuse("a float as a result");
            }

            load("rax", inst.a);
        }

        line("leave");
        line("ret");

        return true;

    default:
        return refuse(std::string("'") + opcode_info(opcode).name + "'");
    }
}

bool X8664Writer::write_binary(const IrInst& inst) {
    u32 width = width_of(inst.type);
    IrOpcode opcode = (IrOpcode) inst.opcode;

    if (opcode == IR_SDIV || opcode == IR_SREM || opcode == IR_UDIV
        || opcode == IR_UREM) {
        write_division(inst, width);

        return true;
    }

    if (opcode == IR_SHL || opcode == IR_LSHR || opcode == IR_ASHR) {
        write_shift(inst, width);

        return true;
    }

    load("rax", inst.a);
    load("rcx", inst.b);

    switch (opcode) {
    case IR_ADD:
        line("add rax, rcx");
        break;

    case IR_SUB:
        line("sub rax, rcx");
        break;

    case IR_MUL:
        line("imul rax, rcx");
        break;

    case IR_AND:
        line("and rax, rcx");
        break;

    case IR_OR:
        line("or rax, rcx");
        break;

    case IR_XOR:
        line("xor rax, rcx");
        break;

    default:
        break;
    }

    mask(width);
    store_result(inst);

    return true;
}

// Question 3: a zero divisor stops the program, and MIN / -1 is MIN, MIN % -1
// is 0, where the machine's 'idiv' would fault. Every width divides in 64 bits
void X8664Writer::write_division(const IrInst& inst, u32 width) {
    IrOpcode opcode = (IrOpcode) inst.opcode;
    bool sign = opcode == IR_SDIV || opcode == IR_SREM;
    bool remainder = opcode == IR_SREM || opcode == IR_UREM;

    load("rax", inst.a);
    load("rcx", inst.b);
    line("test rcx, rcx");
    line("je .Ldivision_by_zero");

    if (sign) {
        std::string divide = fresh_label();
        std::string end = fresh_label();

        widen("rax", width);
        widen("rcx", width);
        line("cmp rcx, -1");
        line("jne " + divide);

        if (remainder) {
            line("xor eax, eax");
        } else {
            line("neg rax");
        }

        line("jmp " + end);
        text << divide << ":\n";
        line("cqo");
        line("idiv rcx");

        if (remainder) {
            line("mov rax, rdx");
        }

        text << end << ":\n";
    } else {
        line("xor edx, edx");
        line("div rcx");

        if (remainder) {
            line("mov rax, rdx");
        }
    }

    mask(width);
    store_result(inst);
}

// Question 2: a count of the width or more gives the arithmetic result, where
// the machine would only read the count's low bits. In 64 bits a count from
// the width up to 63 gives it by itself, so only past 63 is there anything to do
void X8664Writer::write_shift(const IrInst& inst, u32 width) {
    IrOpcode opcode = (IrOpcode) inst.opcode;
    std::string in_range = fresh_label();

    load("rax", inst.a);
    load("rcx", inst.b);

    if (opcode == IR_ASHR) {
        widen("rax", width);
        line("cmp rcx, 63");
        line("jbe " + in_range);
        line("mov ecx, 63");
        text << in_range << ":\n";
        line("sar rax, cl");
    } else {
        std::string end = fresh_label();

        line("cmp rcx, 63");
        line("jbe " + in_range);
        line("xor eax, eax");
        line("jmp " + end);
        text << in_range << ":\n";
        line(opcode == IR_SHL ? "shl rax, cl" : "shr rax, cl");
        text << end << ":\n";
    }

    mask(width);
    store_result(inst);
}

void X8664Writer::write_comparison(const IrInst& inst, u32 width) {
    const char* set = "sete";
    bool sign = false;

    switch ((IrOpcode) inst.opcode) {
    case IR_NE:
        set = "setne";
        break;

    case IR_SLT:
        set = "setl";
        sign = true;
        break;

    case IR_SLE:
        set = "setle";
        sign = true;
        break;

    case IR_SGT:
        set = "setg";
        sign = true;
        break;

    case IR_SGE:
        set = "setge";
        sign = true;
        break;

    case IR_ULT:
        set = "setb";
        break;

    case IR_ULE:
        set = "setbe";
        break;

    case IR_UGT:
        set = "seta";
        break;

    case IR_UGE:
        set = "setae";
        break;

    default:
        break;
    }

    load("rax", inst.a);
    load("rcx", inst.b);

    if (sign) {
        widen("rax", width);
        widen("rcx", width);
    }

    line("cmp rax, rcx");
    line(std::string(set) + " al");
    line("movzx eax, al");
    store_result(inst);
}

// the stack's arguments first, the last of them pushed first, and then the
// registers, so that nothing loading one is in the way of another
bool X8664Writer::write_call(IrFunction& function, IrInstId id) {
    const IrInst& inst = function.get(id);
    IrObject& callee = program->get_object({inst.extra});
    std::vector<u32> arguments = function.pool_of(id);
    u32 on_stack = 0;

    for (u32 argument : arguments) {
        if (is_float(function.get(IrValueId{argument}).type)) {
            return refuse("a float as an argument");
        }
    }

    if (inst.result.index != 0 && is_float(function.get(inst.result).type)) {
        return refuse("a float as a result");
    }

    for (u32 i = (u32) arguments.size(); i > ARGUMENTS_IN_REGISTERS; i--) {
        load("rax", arguments[i - 1]);
        line("push rax");
        on_stack++;
    }

    for (u32 i = 0; i < arguments.size() && i < ARGUMENTS_IN_REGISTERS; i++) {
        load(ARGUMENTS[i], arguments[i]);
    }

    line("call " + quoted(callee.name));

    if (on_stack > 0) {
        line("add rsp, " + std::to_string(on_stack * 8));
    }

    store_result(inst);

    return true;
}
