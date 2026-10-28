#include <haard/ir/ir_verifier.h>

using namespace haard;

// '1 argument', '2 arguments'
static std::string count_of(u32 count, const std::string& thing) {
    return std::to_string(count) + " " + thing + (count == 1 ? "" : "s");
}

IrVerifier::IrVerifier() {
    program = nullptr;
    current.index = 0;
    function = nullptr;
}

void IrVerifier::set_program(IrProgram* program) {
    this->program = program;
}

bool IrVerifier::verify() {
    errors.clear();

    for (u32 i = 1; i < program->get_object_count(); i++) {
        IrObjectId id = {i};

        IrObject& object = program->get_object(id);

        if (object.kind == IR_OBJECT_FUNCTION && !object.native) {
            verify_function(id);
        }

        for (const IrRelocation& one : object.relocations) {
            if (!is_object(one.target.index, IR_OBJECT_NONE)
                || one.offset >= object.bytes.size()) {
                IrVerifierError error;

                error.function = id;
                error.inst.index = 0;
                error.message = "@" + object.name + ": a relocation at "
                              + std::to_string(one.offset)
                              + " names no object or lies outside it";
                errors.push_back(error);
            }
        }
    }

    return errors.size() == 0;
}

const std::vector<IrVerifierError>& IrVerifier::get_errors() {
    return errors;
}

void IrVerifier::verify_function(IrObjectId id) {
    current = id;
    function = &program->get_function(id);

    if (function->form == IR_FORM_STRUCTURED) {
        verify_structured_block(function->body);
    } else {
        verify_cfg();
    }
}

// a block and, through its regions, every block inside it
void IrVerifier::verify_structured_block(IrBlockId block) {
    for (IrInstId id = function->get(block).first; id.index != 0;
         id = function->get(id).next) {
        IrInst& inst = function->get(id);

        verify_inst(id);

        if (inst.opcode == IR_BLOCK || inst.opcode == IR_IF
            || inst.opcode == IR_LOOP) {
            verify_structured_block({inst.extra});

            if (inst.opcode == IR_IF && inst.c != 0) {
                verify_structured_block({inst.c});
            }
        }
    }
}

void IrVerifier::verify_cfg() {
    in_order.clear();

    for (IrBlockId block : function->order) {
        in_order.insert(block.index);
    }

    compute_dominators();

    for (IrBlockId block : function->order) {
        IrBlock& entry = function->get(block);

        if (entry.first.index == 0) {
            IrInstId none = {0};

            report(none, "block ^" + std::to_string(block.index)
                             + " is empty, and a block ends in a terminator");
            continue;
        }

        if (entry.header && function->real_time
            && entry.bound == IR_NO_BOUND) {
            report(entry.first, "the loop at ^" + std::to_string(block.index)
                                    + " is real-time code and has no bound");
        }

        if (!opcode_info((IrOpcode) function->get(entry.last).opcode)
                 .terminator) {
            report(entry.last, "the last instruction of ^"
                                   + std::to_string(block.index)
                                   + " is not a terminator");
        }

        for (IrInstId id = entry.first; id.index != 0;
             id = function->get(id).next) {
            verify_inst(id);

            if (function->get(id).next.index == 0) {
                for (IrBlockId target : successors(block)) {
                    if (target.index == function->order[0].index) {
                        report(id, "nothing may jump to the entry ^"
                                       + std::to_string(target.index));
                    }
                }
            }
        }
    }
}

void IrVerifier::verify_inst(IrInstId id) {
    IrInst& inst = function->get(id);
    IrOpcode opcode = (IrOpcode) inst.opcode;

    if (function->positions[id.index].offset == IR_NO_OFFSET) {
        report(id, "has no position in the source");
    }

    bool jump = opcode == IR_BR || opcode == IR_COND_BR
             || opcode == IR_SWITCH;
    bool region = opcode == IR_BLOCK || opcode == IR_IF || opcode == IR_LOOP
               || opcode == IR_BREAK || opcode == IR_CONTINUE;

    if (function->form == IR_FORM_STRUCTURED && jump) {
        report(id, "a jump of the CFG form in a structured function");
        return;
    }

    if (function->form == IR_FORM_CFG && region) {
        report(id, "a region of the structured form in a CFG");
        return;
    }

    if (opcode_info(opcode).terminator && inst.next.index != 0) {
        report(id, "ends its block, and something follows it");
    }

    if (opcode == IR_LOOP && function->real_time && inst.c == IR_NO_BOUND) {
        report(id, "a loop of real-time code has no bound");
    }

    verify_types(id);

    if (opcode == IR_BREAK || opcode == IR_CONTINUE) {
        verify_jump(id);
    }
}

void IrVerifier::verify_types(IrInstId id) {
    IrInst& inst = function->get(id);
    IrOpcode opcode = (IrOpcode) inst.opcode;
    IrTypeTable* types = program->get_types();
    IrTypeId boolean = types->scalar(IR_BOOL);
    IrTypeId none = {0};

    switch (opcode) {
    case IR_CONST:
        if (!is_integer(inst.type) && !is_float(inst.type)
            && inst.type.index != boolean.index) {
            report(id, "a constant of " + name(inst.type)
                           + ", which is not a number or a bool");
        }

        return;

    case IR_NULL:
        return;

    case IR_ADDR:
        if (!is_object(inst.extra, IR_OBJECT_NONE)) {
            report(id, "names no object");
        }

        return;

    case IR_SIZE:
        if (!is_integer(type_of(inst.result.index))) {
            report(id, "gives a size as " + name(type_of(inst.result.index))
                           + ", which is not a whole number");
        }

        return;

    case IR_ADD:
    case IR_SUB:
    case IR_MUL:
    case IR_SDIV:
    case IR_UDIV:
    case IR_SREM:
    case IR_UREM:
    case IR_SMULHI:
    case IR_UMULHI:
    case IR_SHL:
    case IR_LSHR:
    case IR_ASHR:
        if (!is_integer(inst.type)) {
            report(id, "works on whole numbers, not on " + name(inst.type));
            return;
        }

        operand(id, inst.a, "first", inst.type);
        operand(id, inst.b, "second", inst.type);
        return;

    case IR_AND:
    case IR_OR:
    case IR_XOR:
        if (!is_integer(inst.type) && inst.type.index != boolean.index) {
            report(id, "works on whole numbers and bools, not on "
                           + name(inst.type));
            return;
        }

        operand(id, inst.a, "first", inst.type);
        operand(id, inst.b, "second", inst.type);
        return;

    case IR_NEG:
        if (!is_integer(inst.type)) {
            report(id, "works on whole numbers, not on " + name(inst.type));
            return;
        }

        operand(id, inst.a, "first", inst.type);
        return;

    case IR_NOT:
        if (!is_integer(inst.type) && inst.type.index != boolean.index) {
            report(id, "works on whole numbers and bools, not on "
                           + name(inst.type));
            return;
        }

        operand(id, inst.a, "first", inst.type);
        return;

    case IR_FADD:
    case IR_FSUB:
    case IR_FMUL:
    case IR_FDIV:
    case IR_FLT:
    case IR_FLE:
    case IR_FGT:
    case IR_FGE:
    case IR_FEQ:
    case IR_FNE:
        if (!is_float(inst.type)) {
            report(id, "works on floats, not on " + name(inst.type));
            return;
        }

        operand(id, inst.a, "first", inst.type);
        operand(id, inst.b, "second", inst.type);
        return;

    case IR_FNEG:
        if (!is_float(inst.type)) {
            report(id, "works on floats, not on " + name(inst.type));
            return;
        }

        operand(id, inst.a, "first", inst.type);
        return;

    case IR_EQ:
    case IR_NE:
        if (!is_integer(inst.type) && !is_pointer(inst.type)
            && inst.type.index != boolean.index) {
            report(id, "compares whole numbers, pointers and bools, not "
                           + name(inst.type));
            return;
        }

        operand(id, inst.a, "first", inst.type);
        operand(id, inst.b, "second", inst.type);
        return;

    case IR_SLT:
    case IR_SLE:
    case IR_SGT:
    case IR_SGE:
    case IR_ULT:
    case IR_ULE:
    case IR_UGT:
    case IR_UGE:
        if (!is_integer(inst.type)
            && !(opcode >= IR_ULT && is_pointer(inst.type))) {
            report(id, "orders whole numbers, not " + name(inst.type));
            return;
        }

        operand(id, inst.a, "first", inst.type);
        operand(id, inst.b, "second", inst.type);
        return;

    case IR_TRUNC:
    case IR_ZEXT:
    case IR_SEXT: {
        if (!operand(id, inst.a, "first", none)) {
            return;
        }

        IrTypeId from = type_of(inst.a);
        bool widening = opcode != IR_TRUNC;

        if (!is_integer(inst.type)
            || !(is_integer(from)
                 || (widening && from.index == boolean.index))) {
            report(id, "converts between whole numbers, not from "
                           + name(from) + " to " + name(inst.type));
        } else if (widening ? width(from) >= width(inst.type)
                            : width(from) <= width(inst.type)) {
            report(id, std::string(widening ? "widens" : "narrows")
                           + ", and " + name(from) + " to "
                           + name(inst.type) + " does not");
        }

        return;
    }

    case IR_SITOFP:
    case IR_UITOFP:
    case IR_FPTOSI:
    case IR_FPTOUI:
    case IR_FPEXT:
    case IR_FPTRUNC:
    case IR_PTRTOINT:
    case IR_INTTOPTR: {
        if (!operand(id, inst.a, "first", none)) {
            return;
        }

        IrTypeId from = type_of(inst.a);
        bool right = false;

        switch (opcode) {
        case IR_SITOFP:
        case IR_UITOFP:
            right = is_integer(from) && is_float(inst.type);
            break;

        case IR_FPTOSI:
        case IR_FPTOUI:
            right = is_float(from) && is_integer(inst.type);
            break;

        case IR_FPEXT:
            right = from.index == types->scalar(IR_F32).index
                 && inst.type.index == types->scalar(IR_F64).index;
            break;

        case IR_FPTRUNC:
            right = from.index == types->scalar(IR_F64).index
                 && inst.type.index == types->scalar(IR_F32).index;
            break;

        case IR_PTRTOINT:
            right = is_pointer(from) && is_integer(inst.type);
            break;

        default:
            right = is_integer(from) && is_pointer(inst.type);
            break;
        }

        if (!right) {
            report(id, "cannot convert " + name(from) + " to "
                           + name(inst.type));
        }

        return;
    }

    case IR_SELECT:
        operand(id, inst.a, "condition", boolean);
        operand(id, inst.b, "first", inst.type);
        operand(id, inst.c, "second", inst.type);
        return;

    case IR_SLOT:
        if (inst.type.index == 0) {
            report(id, "holds no type");
        }

        return;

    case IR_LOAD:
    case IR_VOLATILE_LOAD:
        if (!is_scalar(inst.type)) {
            report(id, "loads " + name(inst.type)
                           + ", and an aggregate is never a value");
        }

        operand(id, inst.a, "address", types->pointer(IR_SPACE_GENERIC));
        return;

    case IR_STORE:
    case IR_VOLATILE_STORE:
        if (!is_scalar(inst.type)) {
            report(id, "stores " + name(inst.type)
                           + ", and an aggregate is never a value");
        }

        operand(id, inst.a, "value", inst.type);
        operand(id, inst.b, "address", types->pointer(IR_SPACE_GENERIC));
        return;

    case IR_COPY:
        if (is_scalar(inst.type) || inst.type.index == 0) {
            report(id, "copies " + name(inst.type)
                           + ", which a load and a store would");
        }

        operand(id, inst.a, "destination", types->pointer(IR_SPACE_GENERIC));
        operand(id, inst.b, "source", types->pointer(IR_SPACE_GENERIC));
        return;

    case IR_FIELD:
        if (!types->is_aggregate(inst.type)) {
            report(id, name(inst.type) + " has no fields");
        } else if (inst.extra
                   >= types->get_aggregate(inst.type)->fields.size()) {
            report(id, name(inst.type) + " has no field "
                           + std::to_string(inst.extra));
        }

        operand(id, inst.a, "address", types->pointer(IR_SPACE_GENERIC));
        return;

    case IR_INDEX:
        if (inst.type.index == 0) {
            report(id, "names no element type");
        }

        operand(id, inst.a, "address", types->pointer(IR_SPACE_GENERIC));

        if (operand(id, inst.b, "index", none) && !is_integer(type_of(inst.b))) {
            report(id, "indexes by " + name(type_of(inst.b))
                           + ", which is not a whole number");
        }

        return;

    case IR_CHECK_INDEX:
        if (!is_integer(inst.type)) {
            report(id, "checks whole numbers, not " + name(inst.type));
            return;
        }

        operand(id, inst.a, "index", inst.type);
        operand(id, inst.b, "length", inst.type);
        return;

    case IR_UPCAST:
    case IR_DOWNCAST: {
        IrTypeId to = {inst.extra};
        bool upward = opcode == IR_UPCAST;

        if (!(upward ? is_derived(inst.type, to) : is_derived(to, inst.type))) {
            report(id, name(upward ? to : inst.type) + " is not a base of "
                           + name(upward ? inst.type : to));
        }

        operand(id, inst.a, "address", types->pointer(IR_SPACE_GENERIC));
        return;
    }

    case IR_CALL: {
        if (!is_object(inst.extra, IR_OBJECT_FUNCTION)) {
            report(id, "calls something that is not a function");
            return;
        }

        verify_call(id, function->pool_of(id),
                    program->get_function({inst.extra}).signature);
        return;
    }

    case IR_CALL_INDIRECT:
        operand(id, inst.a, "function", types->pointer(IR_SPACE_GENERIC));
        verify_call(id, function->pool_of(id), inst.extra);
        return;

    case IR_VCALL: {
        const IrClass* entry = program->get_classes()->get_class(inst.type);
        std::vector<u32> arguments = function->pool_of(id);

        if (entry == nullptr) {
            report(id, name(inst.type) + " is not a class");
            return;
        }

        if (inst.extra >= entry->slots.size()) {
            report(id, name(inst.type) + " has no slot "
                           + std::to_string(inst.extra));
            return;
        }

        if (arguments.size() == 0) {
            report(id, "has no receiver");
            return;
        }

        operand(id, arguments[0], "receiver",
                types->pointer(IR_SPACE_GENERIC));

        if (entry->slots[inst.extra].signature != 0
            && entry->slots[inst.extra].signature != inst.c) {
            report(id, "calls slot " + std::to_string(inst.extra)
                           + " with a signature that is not its own");
        }

        if (inst.c != 0) {
            verify_call(id, arguments, inst.c);
        }

        return;
    }

    case IR_IF:
        operand(id, inst.a, "condition", boolean);
        return;

    case IR_BR:
    case IR_COND_BR:
    case IR_SWITCH: {
        std::vector<IrBlockId> targets;

        targets.push_back({inst.extra});

        if (opcode == IR_COND_BR) {
            targets.push_back({inst.b});
            operand(id, inst.a, "condition", boolean);
        }

        if (opcode == IR_SWITCH) {
            std::vector<u32> pool = function->pool_of(id);

            for (u32 i = 1; i < pool.size(); i += 2) {
                targets.push_back({pool[i]});
            }

            if (!is_integer(inst.type)) {
                report(id, "switches on " + name(inst.type)
                               + ", which is not a whole number");
            }

            operand(id, inst.a, "value", inst.type);
        }

        for (IrBlockId target : targets) {
            if (in_order.count(target.index) == 0) {
                report(id, "jumps to ^" + std::to_string(target.index)
                               + ", which is not a block of this function");
                return;
            }
        }

        std::vector<u32> parameters = function->parameters_of({inst.extra});
        std::vector<u32> given = function->pool_of(id);

        if (opcode == IR_BR) {
            if (given.size() != parameters.size()) {
                report(id, "gives " + count_of(given.size(), "argument")
                               + " to ^" + std::to_string(inst.extra)
                               + ", which takes "
                               + std::to_string(parameters.size()));
                return;
            }

            for (u32 i = 0; i < given.size(); i++) {
                operand(id, given[i], "block argument",
                        type_of(parameters[i]));
            }
        } else {
            for (IrBlockId target : targets) {
                if (function->parameters_of(target).size() > 0) {
                    report(id, "jumps to ^" + std::to_string(target.index)
                                   + ", which takes arguments only a 'br' "
                                     "gives");
                }
            }
        }

        return;
    }

    case IR_RET: {
        IrTypeId result = program->get_signature(function->signature).result;

        if (result.index == 0 && inst.a != 0) {
            report(id, "gives a value back from a function that gives none");
        } else if (result.index != 0 && inst.a == 0) {
            report(id, "gives nothing back from a function that gives "
                           + name(result));
        } else if (result.index != 0) {
            operand(id, inst.a, "value", result);
        }

        return;
    }

    case IR_ABORT:
        if (!is_object(inst.extra, IR_OBJECT_DATA)
            || !program->get_object({inst.extra}).read_only) {
            report(id, "its message is not read-only data");
        }

        return;

    default:
        return;
    }
}

void IrVerifier::verify_call(IrInstId id, const std::vector<u32>& arguments,
                             u32 signature) {
    const IrSignature& taken = program->get_signature(signature);

    if (arguments.size() != taken.parameters.size()) {
        report(id, "gives " + count_of(arguments.size(), "argument")
                       + " to a function that takes "
                       + std::to_string(taken.parameters.size()));
        return;
    }

    for (u32 i = 0; i < arguments.size(); i++) {
        operand(id, arguments[i], ("argument " + std::to_string(i + 1)).c_str(),
                taken.parameters[i]);
    }
}

// a 'break' or a 'continue' leaves a region it is inside, and a 'continue'
// goes back to the top of a loop
void IrVerifier::verify_jump(IrInstId id) {
    IrInst& inst = function->get(id);
    u32 target = inst.extra;

    if (target == 0 || target >= function->instructions.size()) {
        report(id, "names no region");
        return;
    }

    u8 kind = function->get(IrInstId{target}).opcode;

    if (kind != IR_BLOCK && kind != IR_IF && kind != IR_LOOP) {
        report(id, "names something that is not a region");
        return;
    }

    if (inst.opcode == IR_CONTINUE && kind != IR_LOOP) {
        report(id, "continues $" + std::to_string(target)
                       + ", which is not a loop");
        return;
    }

    for (IrInstId region = function->get(inst.block).region;
         region.index != 0;
         region = function->get(function->get(region).block).region) {
        if (region.index == target) {
            return;
        }
    }

    report(id, "leaves $" + std::to_string(target) + ", which is not around it");
}

bool IrVerifier::operand(IrInstId id, u32 value, const char* which,
                         IrTypeId type) {
    if (value == 0 || value >= function->values.size()) {
        report(id, std::string("its ") + which + " operand is no value");
        return false;
    }

    if (!is_visible(value, id)) {
        report(id, std::string("its ") + which + " operand %"
                       + std::to_string(value)
                       + " is not defined before it, around it");
        return false;
    }

    if (type.index != 0 && type_of(value).index != type.index) {
        report(id, std::string("its ") + which + " operand is "
                       + name(type_of(value)) + ", not " + name(type));
        return false;
    }

    return true;
}

// The structured form's scope rule: a value is visible after its definition
// in its block and in every region inside it, and never outside it. A CFG
// asks the dominator tree
bool IrVerifier::is_visible(u32 index, IrInstId use) {
    IrValue& value = function->values[index];
    IrBlockId used_in = function->get(use).block;

    if (value.kind == IR_VALUE_PARAMETER) {
        return true;
    }

    if (value.kind == IR_VALUE_BLOCK_PARAMETER) {
        return function->form == IR_FORM_CFG
            && dominates({value.owner}, used_in);
    }

    IrInstId definition = {value.owner};
    IrBlockId defined_in = function->get(definition).block;

    if (function->form == IR_FORM_CFG) {
        return defined_in.index == used_in.index
                   ? comes_before(definition, use)
                   : dominates(defined_in, used_in);
    }

    for (IrInstId at = use; at.index != 0;
         at = function->get(function->get(at).block).region) {
        if (function->get(at).block.index == defined_in.index) {
            return comes_before(definition, at);
        }
    }

    return false;
}

bool IrVerifier::comes_before(IrInstId first, IrInstId second) {
    for (IrInstId at = function->get(second).previous; at.index != 0;
         at = function->get(at).previous) {
        if (at.index == first.index) {
            return true;
        }
    }

    return false;
}

bool IrVerifier::dominates(IrBlockId first, IrBlockId second) {
    return second.index < dominators.size()
        && dominators[second.index].count(first.index) > 0;
}

// the textbook iteration: the entry is dominated by itself, every other block
// by itself and by what dominates all of its predecessors
void IrVerifier::compute_dominators() {
    std::vector<std::vector<u32>> predecessors(function->blocks.size());

    dominators.assign(function->blocks.size(), in_order);

    if (function->order.size() == 0) {
        return;
    }

    for (IrBlockId block : function->order) {
        for (IrBlockId target : successors(block)) {
            if (target.index < predecessors.size()) {
                predecessors[target.index].push_back(block.index);
            }
        }
    }

    u32 entry = function->order[0].index;

    dominators[entry].clear();
    dominators[entry].insert(entry);

    bool changed = true;

    while (changed) {
        changed = false;

        for (IrBlockId block : function->order) {
            if (block.index == entry) {
                continue;
            }

            std::set<u32> next = in_order;

            for (u32 from : predecessors[block.index]) {
                std::set<u32> kept;

                for (u32 one : next) {
                    if (dominators[from].count(one) > 0) {
                        kept.insert(one);
                    }
                }

                next = kept;
            }

            next.insert(block.index);

            if (next != dominators[block.index]) {
                dominators[block.index] = next;
                changed = true;
            }
        }
    }
}

std::vector<IrBlockId> IrVerifier::successors(IrBlockId block) {
    std::vector<IrBlockId> out;
    IrInstId last = function->get(block).last;

    if (last.index == 0) {
        return out;
    }

    IrInst& inst = function->get(last);

    if (inst.opcode == IR_BR || inst.opcode == IR_COND_BR
        || inst.opcode == IR_SWITCH) {
        out.push_back({inst.extra});
    }

    if (inst.opcode == IR_COND_BR) {
        out.push_back({inst.b});
    }

    if (inst.opcode == IR_SWITCH) {
        std::vector<u32> pool = function->pool_of(last);

        for (u32 i = 1; i < pool.size(); i += 2) {
            out.push_back({pool[i]});
        }
    }

    return out;
}

bool IrVerifier::is_object(u32 index, u8 kind) {
    if (index == 0 || index >= program->get_object_count()) {
        return false;
    }

    return kind == IR_OBJECT_NONE
        || program->get_object({index}).kind == kind;
}

bool IrVerifier::is_derived(IrTypeId derived, IrTypeId base) {
    IrTypeTable* types = program->get_types();

    if (!types->is_aggregate(derived) || !types->is_aggregate(base)) {
        return false;
    }

    for (IrTypeId at = types->get_aggregate(derived)->base; at.index != 0;
         at = types->get_aggregate(at)->base) {
        if (at.index == base.index) {
            return true;
        }
    }

    return false;
}

bool IrVerifier::is_integer(IrTypeId type) {
    u8 kind = program->get_types()->get_type(type)->kind;

    return kind >= IR_B8 && kind <= IR_B64;
}

bool IrVerifier::is_float(IrTypeId type) {
    u8 kind = program->get_types()->get_type(type)->kind;

    return kind == IR_F32 || kind == IR_F64;
}

bool IrVerifier::is_pointer(IrTypeId type) {
    return program->get_types()->get_type(type)->kind == IR_PTR;
}

bool IrVerifier::is_scalar(IrTypeId type) {
    u8 kind = program->get_types()->get_type(type)->kind;

    return kind >= IR_B8 && kind <= IR_PTR;
}

// in bits; a bool is one, which is what lets it be widened to any integer
u32 IrVerifier::width(IrTypeId type) {
    switch (program->get_types()->get_type(type)->kind) {
    case IR_BOOL:
        return 1;

    case IR_B8:
        return 8;

    case IR_B16:
        return 16;

    case IR_B32:
        return 32;

    default:
        return 64;
    }
}

IrTypeId IrVerifier::type_of(u32 value) {
    return function->values[value].type;
}

std::string IrVerifier::name(IrTypeId type) {
    return type.index == 0 ? "nothing" : program->get_types()->name_of(type);
}

void IrVerifier::report(IrInstId id, const std::string& message) {
    IrVerifierError error;
    std::string where = "@" + program->get_object(current).name;

    if (id.index != 0) {
        where += ", " + std::string(opcode_info(
                            (IrOpcode) function->get(id).opcode).name)
               + " #" + std::to_string(id.index);
    }

    error.function = current;
    error.inst = id;
    error.message = where + ": " + message;
    errors.push_back(error);
}
