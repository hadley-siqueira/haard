#include <haard/ir/ir_printer.h>

#include <cstdio>
#include <cstring>

using namespace haard;

IrPrinter::IrPrinter() {
    program = nullptr;
    show_positions = false;
    marked_function.index = 0;
    marked.index = 0;
}

void IrPrinter::set_program(IrProgram* program) {
    this->program = program;
}

void IrPrinter::set_show_positions(bool show) {
    show_positions = show;
}

void IrPrinter::set_mark(IrObjectId function, IrInstId inst) {
    marked_function = function;
    marked = inst;
}

// the types, the data, then each function, a blank line between any two
void IrPrinter::print(std::ostream& out) {
    bool written = print_types(out);
    bool data = false;

    for (u32 i = 1; i < program->get_object_count(); i++) {
        IrObjectId id = {i};

        if (program->get_object(id).kind == IR_OBJECT_DATA) {
            out << (written && !data ? "\n" : "");
            print_data(out, id);
            data = true;
        }
    }

    written = written || data;

    for (u32 i = 1; i < program->get_object_count(); i++) {
        IrObjectId id = {i};

        if (program->get_object(id).kind == IR_OBJECT_FUNCTION) {
            out << (written ? "\n" : "");
            print_function(out, id);
            written = true;
        }
    }
}

// the aggregates, and a class's vtable after it; whether there were any
bool IrPrinter::print_types(std::ostream& out) {
    IrTypeTable* types = program->get_types();
    bool any = false;

    for (u32 i = 1; i < types->get_count(); i++) {
        IrTypeId id = {i};

        if (!types->is_aggregate(id)) {
            continue;
        }

        IrAggregate* aggregate = types->get_aggregate(id);
        u8 kind = types->get_type(id)->kind;

        out << (kind == IR_CLASS   ? "class "
                : kind == IR_UNION ? "union "
                                   : "struct ")
            << aggregate->name;

        if (aggregate->base.index != 0) {
            out << " : " << type(aggregate->base);
        }

        out << " {";

        for (u32 f = 0; f < aggregate->fields.size(); f++) {
            out << (f > 0 ? ", " : " ") << aggregate->fields[f].name << ": "
                << type(aggregate->fields[f].type);
        }

        out << (aggregate->fields.size() > 0 ? " }" : "}") << "\n";

        const IrClass* entry = program->get_classes()->get_class(id);

        if (entry != nullptr) {
            out << "vtable " << aggregate->name << " {";

            for (u32 s = 0; s < entry->slots.size(); s++) {
                out << (s > 0 ? ", " : " ") << s << ": "
                    << entry->slots[s].name;
            }

            out << " }\n";
        }

        any = true;
    }

    return any;
}

void IrPrinter::print_data(std::ostream& out, IrObjectId id) {
    IrObject& object = program->get_object(id);

    out << "data @" << object.name << " : " << type(object.type) << " = "
        << (object.read_only ? "readonly " : "");

    if (object.zeroed) {
        out << "zero\n";
        return;
    }

    out << "\"";

    for (u8 byte : object.bytes) {
        if (byte >= 32 && byte < 127 && byte != '"' && byte != '\\') {
            out << (char) byte;
        } else {
            char escaped[8];

            std::snprintf(escaped, sizeof(escaped), "\\%02x", byte);
            out << escaped;
        }
    }

    out << "\"";

    for (u32 i = 0; i < object.relocations.size(); i++) {
        out << (i == 0 ? " with " : ", ") << "@"
            << program->get_object(object.relocations[i].target).name
            << " at " << object.relocations[i].offset;
    }

    out << "\n";
}

void IrPrinter::print_function(std::ostream& out, IrObjectId id) {
    IrFunction& function = program->get_function(id);
    const IrSignature& taken = program->get_signature(function.signature);

    out << (program->get_object(id).native ? "native @" : "func @")
        << program->get_object(id).name << "(";

    for (u32 i = 0; i < taken.parameters.size(); i++) {
        out << (i > 0 ? ", " : "") << value(function.parameter(i)) << ": "
            << type(taken.parameters[i]);
    }

    out << ")";

    if (taken.result.index != 0) {
        out << " -> " << type(taken.result);
    }

    if (program->get_object(id).native) {
        out << "\n";
        return;
    }

    out << (function.real_time ? " realtime" : "")
        << (program->is_initialiser(id) ? " init" : "") << " {\n";

    if (function.form == IR_FORM_STRUCTURED) {
        print_block(out, function, function.body, 1);
    } else {
        for (IrBlockId block : function.order) {
            std::vector<u32> parameters = function.parameters_of(block);

            out << block_name(function, block);

            if (parameters.size() > 0) {
                out << "(";

                for (u32 i = 0; i < parameters.size(); i++) {
                    out << (i > 0 ? ", " : "") << value(parameters[i]) << ": "
                        << type(function.values[parameters[i]].type);
                }

                out << ")";
            }

            if (function.get(block).header) {
                out << " loop";

                if (function.get(block).bound != IR_NO_BOUND) {
                    out << " bound " << function.get(block).bound;
                }
            }

            out << ":\n";
            print_block(out, function, block, 1);
        }
    }

    out << "}\n";
}

void IrPrinter::print_block(std::ostream& out, IrFunction& function,
                            IrBlockId block, u32 depth) {
    for (IrInstId id = function.get(block).first; id.index != 0;
         id = function.get(id).next) {
        print_inst(out, function, id, depth);
    }
}

void IrPrinter::print_inst(std::ostream& out, IrFunction& function,
                           IrInstId id, u32 depth) {
    IrInst& inst = function.get(id);
    IrOpcode opcode = (IrOpcode) inst.opcode;
    std::string name = opcode_info(opcode).name;
    std::string indent((depth * 2), ' ');
    std::string line;

    if (function.object.index == marked_function.index
        && id.index == marked.index) {
        indent = ">>" + indent.substr(2);
    }

    if (inst.result.index != 0) {
        line = value(inst.result) + " = ";
    }

    switch (opcode) {
    case IR_CONST:
        line += name + " " + type(inst.type) + " "
              + constant(function, inst.type, inst.extra);
        break;

    case IR_NULL:
        line += name;
        break;

    case IR_ADDR:
        line += name + " @" + program->get_object({inst.extra}).name;
        break;

    case IR_SIZE:
        line += name + " " + type(inst.type) + " as "
              + type(function.values[inst.result.index].type);
        break;

    case IR_NEG:
    case IR_NOT:
    case IR_FNEG:
        line += name + " " + type(inst.type) + " " + value(inst.a);
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
        line += name + " " + type(function.values[inst.a].type) + " "
              + value(inst.a) + " to " + type(inst.type);
        break;

    case IR_SELECT:
        line += name + " " + type(inst.type) + " " + value(inst.a) + ", "
              + value(inst.b) + ", " + value(inst.c);
        break;

    case IR_SLOT:
        line += name + " " + type(inst.type);
        break;

    case IR_LOAD:
    case IR_VOLATILE_LOAD:
        line += name + " " + type(inst.type) + ", " + value(inst.a);
        break;

    case IR_STORE:
    case IR_VOLATILE_STORE:
        line += name + " " + type(inst.type) + " " + value(inst.a) + ", "
              + value(inst.b);
        break;

    case IR_COPY:
        line += name + " " + type(inst.type) + " " + value(inst.a) + ", "
              + value(inst.b);
        break;

    case IR_FIELD: {
        IrTypeTable* types = program->get_types();
        std::string field = "?";

        if (types->is_aggregate(inst.type)
            && inst.extra < types->get_aggregate(inst.type)->fields.size()) {
            field = types->get_aggregate(inst.type)->fields[inst.extra].name;
        }

        line += name + " " + value(inst.a) + ", " + type(inst.type) + "."
              + field;
        break;
    }

    case IR_INDEX:
        line += name + " " + value(inst.a) + ", " + value(inst.b) + " : "
              + type(inst.type);
        break;

    case IR_CHECK_INDEX:
        line += name + " " + type(inst.type) + " " + value(inst.a) + ", "
              + value(inst.b);
        break;

    case IR_UPCAST:
    case IR_DOWNCAST:
        line += name + " " + value(inst.a) + ", " + type(inst.type) + " -> "
              + type({inst.extra});
        break;

    case IR_CALL:
        line += name + " @" + program->get_object({inst.extra}).name + "("
              + arguments(function, id) + ")";
        break;

    case IR_CALL_INDIRECT:
        line += name + " " + value(inst.a) + "(" + arguments(function, id)
              + ") : " + signature(inst.extra);
        break;

    case IR_VCALL:
        line += name + " " + type(inst.type) + "."
              + slot_name(inst.type, inst.extra) + "("
              + arguments(function, id) + ")";
        break;

    case IR_FENCE:
        line += name;
        break;

    case IR_BLOCK:
        out << indent << "$" << id.index << ": block {" << where(function, id)
            << "\n";
        print_block(out, function, {inst.extra}, depth + 1);
        out << std::string(depth * 2, ' ') << "}\n";
        return;

    case IR_IF:
        out << indent << "$" << id.index << ": if " << value(inst.a) << " {"
            << where(function, id) << "\n";
        print_block(out, function, {inst.extra}, depth + 1);

        if (inst.c != 0) {
            out << std::string(depth * 2, ' ') << "} else {\n";
            print_block(out, function, {inst.c}, depth + 1);
        }

        out << std::string(depth * 2, ' ') << "}\n";
        return;

    case IR_LOOP:
        out << indent << "$" << id.index << ": loop";

        if (inst.c != IR_NO_BOUND) {
            out << " bound " << inst.c;
        }

        out << " {" << where(function, id) << "\n";
        print_block(out, function, {inst.extra}, depth + 1);
        out << std::string(depth * 2, ' ') << "}\n";
        return;

    case IR_BREAK:
    case IR_CONTINUE:
        line += name + " $" + std::to_string(inst.extra);
        break;

    case IR_BR: {
        std::string passed = arguments(function, id);

        line += name + " " + block_name(function, {inst.extra})
              + (passed.size() > 0 ? "(" + passed + ")" : "");
        break;
    }

    case IR_COND_BR:
        line += name + " " + value(inst.a) + ", "
              + block_name(function, {inst.extra}) + ", "
              + block_name(function, {inst.b});
        break;

    case IR_SWITCH: {
        std::vector<u32> pool = function.pool_of(id);

        line += name + " " + type(inst.type) + " " + value(inst.a)
              + ", default " + block_name(function, {inst.extra}) + " [";

        for (u32 i = 0; i + 1 < pool.size(); i += 2) {
            line += (i > 0 ? ", " : "") + constant(function, inst.type, pool[i])
                  + ": " + block_name(function, {pool[i + 1]});
        }

        line += "]";
        break;
    }

    case IR_RET:
        line += name;

        if (inst.a != 0) {
            line += " " + type(inst.type) + " " + value(inst.a);
        }

        break;

    case IR_ABORT:
        line += name + " @" + program->get_object({inst.extra}).name;
        break;

    // the two operands of an arithmetic, a bitwise operation or a comparison
    default:
        line += name + " " + type(inst.type) + " " + value(inst.a) + ", "
              + value(inst.b);
        break;
    }

    out << indent << line << where(function, id) << "\n";
}

std::string IrPrinter::where(IrFunction& function, IrInstId id) {
    if (!show_positions) {
        return "";
    }

    IrPosition at = function.positions[id.index];

    return "  ; at " + (at.offset == IR_NO_OFFSET
                            ? std::string("nowhere")
                            : std::to_string(at.module) + ":"
                                  + std::to_string(at.offset));
}

std::string IrPrinter::value(IrValueId id) {
    return value(id.index);
}

std::string IrPrinter::value(u32 index) {
    return index == 0 ? "%none" : "%" + std::to_string(index);
}

std::string IrPrinter::type(IrTypeId id) {
    return id.index == 0 ? "none" : program->get_types()->name_of(id);
}

std::string IrPrinter::block_name(IrFunction& function, IrBlockId id) {
    return "^" + std::to_string(id.index);
}

// written as what it means: a bool as a word, a float as its value, and a
// whole number as its bits read unsigned, since the IR's integers carry no
// sign (question 79)
std::string IrPrinter::constant(IrFunction& function, IrTypeId type,
                                u32 index) {
    u64 bits = function.constants[index];
    char text[64];

    switch (program->get_types()->get_type(type)->kind) {
    case IR_BOOL:
        return bits != 0 ? "true" : "false";

    case IR_F32: {
        u32 narrow = (u32) bits;
        float value;

        std::memcpy(&value, &narrow, sizeof(value));
        std::snprintf(text, sizeof(text), "%.9g", value);
        return text;
    }

    case IR_F64: {
        double value;

        std::memcpy(&value, &bits, sizeof(value));
        std::snprintf(text, sizeof(text), "%.17g", value);
        return text;
    }

    default:
        return std::to_string(bits);
    }
}

std::string IrPrinter::arguments(IrFunction& function, IrInstId id) {
    std::vector<u32> pool = function.pool_of(id);
    std::string out;

    for (u32 i = 0; i < pool.size(); i++) {
        out += (i > 0 ? ", " : "") + value(pool[i]);
    }

    return out;
}

std::string IrPrinter::signature(u32 index) {
    const IrSignature& taken = program->get_signature(index);
    std::string out = "(";

    for (u32 i = 0; i < taken.parameters.size(); i++) {
        out += (i > 0 ? ", " : "") + type(taken.parameters[i]);
    }

    out += ")";

    if (taken.result.index != 0) {
        out += " -> " + type(taken.result);
    }

    return out;
}

std::string IrPrinter::slot_name(IrTypeId type, u32 slot) {
    const IrClass* entry = program->get_classes()->get_class(type);

    if (entry == nullptr || slot >= entry->slots.size()) {
        return "slot" + std::to_string(slot);
    }

    return entry->slots[slot].name;
}
