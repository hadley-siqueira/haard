// The IR's own cases, built by hand with the IrBuilder (record 0077, stage
// 2): there is no lowering yet, and question 88 asked for the verifier's
// cases to be IR broken on purpose, which only a builder can make.
//
//   cases --list      every case's name
//   cases <name>      the program printed, then what the verifier says;
//                     for a program the verifier passes, the same after
//                     flatten (stage 3), and when it has a @main, what
//                     running it in the interpreter printed and gave back
//
// A case whose name starts with 'verifier_' breaks one rule, and its golden
// holds the message and the program with the culprit marked '>>'. Every other
// case is well formed and its golden ends in 'verifier: ok'.
//
// One instruction per statement wherever the order matters: C++ leaves the
// order in which a call's arguments are evaluated unspecified, so two
// instructions made inside one argument list could come out either way.
#include <haard/ir/ir_builder.h>
#include <haard/ir/ir_flattener.h>
#include <haard/ir/ir_interpreter.h>
#include <haard/ir/ir_printer.h>
#include <haard/ir/ir_verifier.h>

#include <cstring>
#include <iostream>
#include <sstream>

using namespace haard;

static const IrTypeId NOTHING = {0};

struct Fixture {
    IrProgram program;
    IrBuilder build;
    IrTypeTable* types;
    u32 offset;
    bool positions;

    // the target the program runs on, and @print with the natives it uses
    std::string target;
    IrObjectId write;
    IrObjectId standard_output;
    IrObjectId printer;

    IrTypeId b8, b16, b32, b64, boolean, f32, f64, ptr;

    Fixture() {
        build.set_program(&program);
        types = program.get_types();
        offset = 0;
        positions = false;
        target = "x86_64-linux";
        write.index = 0;
        standard_output.index = 0;
        printer.index = 0;
        b8 = types->scalar(IR_B8);
        b16 = types->scalar(IR_B16);
        b32 = types->scalar(IR_B32);
        b64 = types->scalar(IR_B64);
        boolean = types->scalar(IR_BOOL);
        f32 = types->scalar(IR_F32);
        f64 = types->scalar(IR_F64);
        ptr = types->pointer(IR_SPACE_GENERIC);
    }

    // every instruction somewhere of its own in an imaginary source
    IrPosition at() {
        IrPosition here = {0, offset};

        offset += 4;

        return here;
    }

    IrObjectId function(const std::string& name,
                        const std::vector<IrTypeId>& parameters,
                        IrTypeId result, IrForm form) {
        IrObjectId made = program.add_function(
            name, program.signature(parameters, result), form);

        build.begin_function(made);

        return made;
    }

    IrValueId number(IrTypeId type, u64 value) {
        return build.constant(type, value, at());
    }

    // @print writes a b64 in decimal, signed, and a newline, through the
    // natives of 'std.low_io'. Written in IR by hand, so a run that prints
    // already exercises loops, breaks, 'index' and native calls. Made before
    // the case starts its own functions, since it moves the builder
    void need_print() {
        standard_output = program.add_native("__io_stdout",
                                             program.signature({}, ptr));
        write = program.add_native("__io_write",
                                   program.signature({ptr, b8}, NOTHING));
        printer = function("print", {b64}, NOTHING, IR_FORM_STRUCTURED);

        IrValueId n = build.parameter(0);
        IrValueId handle = build.call(standard_output, {}, at());
        IrValueId zero = number(b64, 0);
        IrValueId one = number(b64, 1);
        IrValueId ten = number(b64, 10);
        IrValueId twenty = number(b64, 20);
        IrValueId digit_zero = number(b8, '0');
        IrValueId left = build.slot(b64, at());
        IrValueId buffer = build.slot(types->array(b8, 20), at());
        IrValueId next = build.slot(b64, at());
        IrValueId negative = build.binary(IR_SLT, b64, n, zero, at());

        build.store(b64, n, left, at());
        build.store(b64, twenty, next, at());
        build.begin_if(negative, at());

        IrValueId minus = number(b8, '-');

        build.call(write, {handle, minus}, at());
        build.store(b64, build.unary(IR_NEG, b64, n, at()), left, at());
        build.end();

        IrInstId digits = build.begin_loop(20, at());
        IrValueId rest = build.load(b64, left, at());
        IrValueId low = build.binary(IR_UREM, b64, rest, ten, at());
        IrValueId place = build.binary(IR_SUB, b64,
                                       build.load(b64, next, at()), one,
                                       at());
        IrValueId narrow = build.convert(IR_TRUNC, b8, low, at());
        IrValueId glyph = build.binary(IR_ADD, b8, narrow, digit_zero, at());
        IrValueId shorter = build.binary(IR_UDIV, b64, rest, ten, at());

        build.store(b64, place, next, at());
        build.store(b8, glyph, build.index(b8, buffer, place, at()), at());
        build.store(b64, shorter, left, at());
        build.begin_if(build.binary(IR_EQ, b64, shorter, zero, at()), at());
        build.break_to(digits, at());
        build.end();
        build.end();

        IrInstId emit = build.begin_loop(20, at());
        IrValueId at_now = build.load(b64, next, at());

        build.begin_if(build.binary(IR_UGE, b64, at_now, twenty, at()), at());
        build.break_to(emit, at());
        build.end();

        IrValueId shown = build.load(b8, build.index(b8, buffer, at_now, at()),
                                     at());

        build.call(write, {handle, shown}, at());
        build.store(b64, build.binary(IR_ADD, b64, at_now, one, at()), next,
                    at());
        build.end();

        IrValueId newline = number(b8, '\n');

        build.call(write, {handle, newline}, at());
        build.ret(at());
    }

    void print(IrValueId value) {
        build.call(printer, {value}, at());
    }

    // a b32 printed as the signed number it holds
    void print_b32(IrValueId value) {
        print(build.convert(IR_SEXT, b64, value, at()));
    }
};

static void every_integer_operation(Fixture& f) {
    f.function("ints", {f.b32, f.b32}, f.b32, IR_FORM_STRUCTURED);

    IrValueId x = f.build.parameter(0);
    IrValueId y = f.build.parameter(1);
    IrOpcode binary[] = {IR_ADD,    IR_SUB,    IR_MUL, IR_SDIV, IR_UDIV,
                         IR_SREM,   IR_UREM,   IR_SMULHI, IR_UMULHI,
                         IR_AND,    IR_OR,     IR_XOR, IR_SHL,  IR_LSHR,
                         IR_ASHR};
    IrValueId last = x;

    for (IrOpcode one : binary) {
        last = f.build.binary(one, f.b32, last, y, f.at());
    }

    last = f.build.unary(IR_NEG, f.b32, last, f.at());
    last = f.build.unary(IR_NOT, f.b32, last, f.at());

    IrOpcode compare[] = {IR_EQ,  IR_NE,  IR_SLT, IR_SLE, IR_SGT,
                          IR_SGE, IR_ULT, IR_ULE, IR_UGT, IR_UGE};

    for (IrOpcode one : compare) {
        f.build.binary(one, f.b32, last, y, f.at());
    }

    IrValueId same = f.build.binary(IR_EQ, f.b32, x, y, f.at());
    IrValueId other = f.build.binary(IR_NE, f.b32, x, y, f.at());
    IrValueId both = f.build.binary(IR_AND, f.boolean, same, other, f.at());

    f.build.unary(IR_NOT, f.boolean, both, f.at());
    f.build.ret(f.b32, last, f.at());
}

static void floats_and_conversions(Fixture& f) {
    f.function("floats", {f.f32, f.b8}, f.f64, IR_FORM_STRUCTURED);

    IrValueId x = f.build.parameter(0);
    IrValueId byte = f.build.parameter(1);
    IrValueId s = f.build.binary(IR_FADD, f.f32, x, x, f.at());

    s = f.build.binary(IR_FSUB, f.f32, s, x, f.at());
    s = f.build.binary(IR_FMUL, f.f32, s, x, f.at());
    s = f.build.binary(IR_FDIV, f.f32, s, x, f.at());
    s = f.build.unary(IR_FNEG, f.f32, s, f.at());

    IrOpcode compare[] = {IR_FEQ, IR_FNE, IR_FLT, IR_FLE, IR_FGT, IR_FGE};

    for (IrOpcode one : compare) {
        f.build.binary(one, f.f32, s, x, f.at());
    }

    IrValueId wide = f.build.convert(IR_ZEXT, f.b32, byte, f.at());
    IrValueId signed_wide = f.build.convert(IR_SEXT, f.b64, byte, f.at());

    f.build.convert(IR_TRUNC, f.b16, wide, f.at());
    f.build.convert(IR_SITOFP, f.f64, signed_wide, f.at());
    f.build.convert(IR_UITOFP, f.f32, wide, f.at());
    f.build.convert(IR_FPTOSI, f.b32, s, f.at());
    f.build.convert(IR_FPTOUI, f.b64, s, f.at());

    IrValueId cell = f.build.slot(f.b64, f.at());
    IrValueId as_number = f.build.convert(IR_PTRTOINT, f.b64, cell, f.at());

    f.build.convert(IR_INTTOPTR, f.ptr, as_number, f.at());

    IrValueId flag = f.build.binary(IR_FLT, f.f32, s, x, f.at());
    IrValueId pick = f.build.select(f.f32, flag, s, x, f.at());

    f.build.convert(IR_FPTRUNC, f.f32,
                    f.build.convert(IR_FPEXT, f.f64, pick, f.at()), f.at());
    f.build.ret(f.f64, f.build.convert(IR_FPEXT, f.f64, pick, f.at()),
                f.at());
}

static void constants_and_objects(Fixture& f) {
    IrObjectId counter = f.program.add_zeroed("counter", f.b64);
    IrObjectId bytes = f.program.add_data(
        "table", f.types->array(f.b8, 3), {1, 2, 255}, false);

    f.program.add_text("greeting", "hi \"you\"\n");
    f.function("constants", {}, f.b64, IR_FORM_STRUCTURED);

    double half = 0.5;
    float third = 1.0f / 3.0f;
    u64 bits = 0;
    u32 narrow = 0;

    std::memcpy(&bits, &half, sizeof(half));
    std::memcpy(&narrow, &third, sizeof(third));

    f.number(f.b8, 255);
    f.number(f.b64, 0xffffffffffffffffull);
    f.number(f.boolean, 1);
    f.number(f.boolean, 0);
    f.number(f.f64, bits);
    f.number(f.f32, narrow);
    f.build.null(f.at());
    f.build.address(bytes, f.at());

    IrValueId where = f.build.address(counter, f.at());
    IrValueId size = f.build.size(f.types->array(f.b32, 10), f.b64, f.at());

    f.build.store(f.b64, size, where, f.at());
    f.build.ret(f.b64, f.build.load(f.b64, where, f.at()), f.at());
}

// two structs and a class hierarchy, by hand, with Square's vtable
static IrTypeId shapes(Fixture& f, IrTypeId& point, IrTypeId& square) {
    IrTypeId shape = f.types->aggregate(IR_CLASS, "Shape");
    IrField field;

    point = f.types->aggregate(IR_STRUCT, "Point");
    square = f.types->aggregate(IR_CLASS, "Square");
    field.candidate = 0;

    field.name = "x";
    field.type = f.b32;
    f.types->get_aggregate(point)->fields.push_back(field);
    field.name = "y";
    f.types->get_aggregate(point)->fields.push_back(field);

    field.name = "origin";
    field.type = point;
    f.types->get_aggregate(shape)->fields.push_back(field);

    f.types->get_aggregate(square)->base = shape;
    field.name = "side";
    field.type = f.b32;
    f.types->get_aggregate(square)->fields.push_back(field);

    u32 area = f.program.signature({f.ptr}, f.b32);
    IrClass made;
    IrSlot slot;

    slot.module = 0;
    slot.candidate = 0;
    slot.name = "destroy";
    slot.implemented_by = shape;
    slot.signature = f.program.signature({f.ptr}, NOTHING);
    made.type = shape;
    made.slots.push_back(slot);
    slot.name = "area";
    slot.signature = area;
    made.slots.push_back(slot);
    f.program.get_classes()->add_class(made);

    made.type = square;
    made.slots[1].implemented_by = square;
    f.program.get_classes()->add_class(made);

    return shape;
}

static void memory_and_aggregates(Fixture& f) {
    IrTypeId point;
    IrTypeId square;
    IrTypeId shape = shapes(f, point, square);

    f.function("memory", {f.ptr}, f.b32, IR_FORM_STRUCTURED);

    IrValueId given = f.build.parameter(0);
    IrValueId here = f.build.slot(point, f.at());
    IrValueId there = f.build.slot(point, f.at());

    f.build.copy(point, there, here, f.at());

    IrValueId x = f.build.field(point, here, 0, f.at());

    f.build.store(f.b32, f.number(f.b32, 7), x, f.at());

    IrValueId row = f.build.slot(f.types->array(f.b32, 4), f.at());
    IrValueId third = f.build.index(f.b32, row, f.number(f.b64, 2), f.at());

    f.build.store(f.b32, f.build.load(f.b32, x, f.at()), third, f.at());

    IrValueId as_shape = f.build.upcast(square, shape, given, f.at());
    IrValueId back = f.build.downcast(shape, square, as_shape, f.at());
    IrValueId side = f.build.field(square, back, 0, f.at());

    f.build.ret(f.b32, f.build.load(f.b32, side, f.at()), f.at());
}

static void calls(Fixture& f) {
    IrTypeId point;
    IrTypeId square;
    IrTypeId shape = shapes(f, point, square);
    IrObjectId twice = f.function("twice", {f.b32}, f.b32, IR_FORM_STRUCTURED);

    f.build.ret(f.b32,
                f.build.binary(IR_ADD, f.b32, f.build.parameter(0),
                               f.build.parameter(0), f.at()),
                f.at());

    IrObjectId nothing = f.function("nothing", {}, NOTHING,
                                    IR_FORM_STRUCTURED);

    f.build.ret(f.at());
    f.function("caller", {f.ptr}, f.b32, IR_FORM_STRUCTURED);

    IrValueId once = f.build.call(twice, {f.number(f.b32, 3)}, f.at());

    f.build.call(nothing, {}, f.at());

    IrValueId pointer = f.build.address(twice, f.at());
    IrValueId again = f.build.call_indirect(f.program.signature({f.b32}, f.b32),
                                            pointer, {once}, f.at());
    IrValueId area = f.build.vcall(shape, 1, f.program.signature({f.ptr},
                                                                 f.b32),
                                   {f.build.parameter(0)}, f.at());

    f.build.ret(f.b32, f.build.binary(IR_ADD, f.b32, again, area, f.at()),
                f.at());
}

static void barriers(Fixture& f) {
    f.function("device", {f.ptr}, f.b32, IR_FORM_STRUCTURED);

    IrValueId port = f.build.parameter(0);

    f.build.volatile_store(f.b32, f.number(f.b32, 1), port, f.at());
    f.build.fence(f.at());
    f.build.ret(f.b32, f.build.volatile_load(f.b32, port, f.at()), f.at());
}

static void regions_nest(Fixture& f) {
    f.function("count", {f.b32}, f.b32, IR_FORM_STRUCTURED);

    IrValueId limit = f.build.parameter(0);
    IrValueId i = f.build.slot(f.b32, f.at());

    f.build.store(f.b32, f.number(f.b32, 0), i, f.at());

    IrInstId outer = f.build.begin_block(f.at());
    IrInstId loop = f.build.begin_loop(100, f.at());
    IrValueId now = f.build.load(f.b32, i, f.at());
    IrValueId done = f.build.binary(IR_SGE, f.b32, now, limit, f.at());

    f.build.begin_if(done, f.at());
    f.build.break_to(outer, f.at());
    f.build.end();

    IrValueId one = f.number(f.b32, 1);
    IrValueId low = f.build.binary(IR_AND, f.b32, now, one, f.at());
    IrValueId odd = f.build.binary(IR_EQ, f.b32, low, one, f.at());
    IrValueId next = f.build.binary(IR_ADD, f.b32, now, one, f.at());

    f.build.store(f.b32, next, i, f.at());
    f.build.begin_if(odd, f.at());
    f.build.continue_to(loop, f.at());
    f.build.begin_else();
    f.build.store(f.b32, now, i, f.at());
    f.build.end();
    f.build.end();
    f.build.end();
    IrInstId spin = f.build.begin_loop(IR_NO_BOUND, f.at());

    f.build.begin_block(f.at());
    f.build.break_to(spin, f.at());
    f.build.end();
    f.build.end();
    f.build.ret(f.b32, f.build.load(f.b32, i, f.at()), f.at());
}

static void a_cfg_with_block_parameters(Fixture& f) {
    f.function("sum", {f.b32}, f.b32, IR_FORM_CFG);

    IrValueId n = f.build.parameter(0);
    IrBlockId head = f.build.new_block({f.b32, f.b32});
    IrBlockId body = f.build.new_block({});
    IrBlockId exit = f.build.new_block({});
    IrBlockId odd = f.build.new_block({});

    f.build.br(head, {f.number(f.b32, 0), f.number(f.b32, 0)}, f.at());

    f.build.set_block(head);

    IrValueId i = f.build.block_parameter(head, 0);
    IrValueId total = f.build.block_parameter(head, 1);

    f.build.cond_br(f.build.binary(IR_SLT, f.b32, i, n, f.at()), body, exit,
                    f.at());

    f.build.set_block(body);
    f.build.switch_on(f.b32, i, odd, {{0, exit}, {7, odd}}, f.at());

    f.build.set_block(odd);
    f.build.br(head,
               {f.build.binary(IR_ADD, f.b32, i, f.number(f.b32, 1), f.at()),
                f.build.binary(IR_ADD, f.b32, total, i, f.at())},
               f.at());

    f.build.set_block(exit);
    f.build.ret(f.b32, n, f.at());
}

static void abort_and_positions(Fixture& f) {
    IrObjectId message = f.program.add_text("unwrapped", "unwrap of None");

    f.positions = true;
    f.function("unwrap", {f.boolean}, f.b32, IR_FORM_STRUCTURED);
    f.build.begin_if(f.build.parameter(0), f.at());
    f.build.abort(message, f.at());
    f.build.end();

    IrValueId one = f.number(f.b32, 1);

    f.build.ret(f.b32, one, f.at());
}

// question 85: the table every pass asks, printed whole
static void every_opcode_has_its_effects(Fixture& f) {
    for (u32 i = 1; i < IR_OP_COUNT; i++) {
        const IrOpcodeInfo& info = opcode_info((IrOpcode) i);

        std::cout << info.name << ":"
                  << (info.effects & IR_EFFECT_READS ? " reads" : "")
                  << (info.effects & IR_EFFECT_WRITES ? " writes" : "")
                  << (info.effects & IR_EFFECT_TRAPS ? " traps" : "")
                  << (info.effects & IR_EFFECT_BARRIER ? " barrier" : "")
                  << (info.terminator ? " terminator" : "") << "\n";
    }

    std::cout << "\n";
}

// ---- the verifier, one rule broken per case ----

static void verifier_a_jump_in_a_structured_function(Fixture& f) {
    f.function("f", {}, NOTHING, IR_FORM_STRUCTURED);
    f.build.br(IrBlockId{1}, {}, f.at());
}

static void verifier_a_region_in_a_cfg(Fixture& f) {
    f.function("f", {}, NOTHING, IR_FORM_CFG);
    f.build.begin_block(f.at());
    f.build.end();
    f.build.ret(f.at());
}

static void verifier_something_after_a_terminator(Fixture& f) {
    f.function("f", {}, NOTHING, IR_FORM_STRUCTURED);
    f.build.ret(f.at());
    f.build.fence(f.at());
}

static void verifier_a_block_that_does_not_end(Fixture& f) {
    f.function("f", {}, NOTHING, IR_FORM_CFG);
    f.build.fence(f.at());
}

static void verifier_an_empty_block(Fixture& f) {
    f.function("f", {}, NOTHING, IR_FORM_CFG);
    f.build.br(f.build.new_block({}), {}, f.at());
}

static void verifier_a_jump_to_the_entry(Fixture& f) {
    f.function("f", {}, NOTHING, IR_FORM_CFG);
    f.build.br(IrBlockId{1}, {}, f.at());
}

static void verifier_a_jump_out_of_the_function(Fixture& f) {
    f.function("f", {}, NOTHING, IR_FORM_CFG);
    f.build.br(IrBlockId{9}, {}, f.at());
}

static void verifier_too_few_block_arguments(Fixture& f) {
    f.function("f", {}, NOTHING, IR_FORM_CFG);

    IrBlockId next = f.build.new_block({f.b32});

    f.build.br(next, {}, f.at());
    f.build.set_block(next);
    f.build.ret(f.at());
}

static void verifier_a_block_argument_of_the_wrong_type(Fixture& f) {
    f.function("f", {}, NOTHING, IR_FORM_CFG);

    IrBlockId next = f.build.new_block({f.b32});

    f.build.br(next, {f.number(f.b64, 1)}, f.at());
    f.build.set_block(next);
    f.build.ret(f.at());
}

static void verifier_cond_br_to_a_block_with_parameters(Fixture& f) {
    f.function("f", {f.boolean}, NOTHING, IR_FORM_CFG);

    IrBlockId next = f.build.new_block({f.b32});
    IrBlockId other = f.build.new_block({});

    f.build.cond_br(f.build.parameter(0), next, other, f.at());
    f.build.set_block(next);
    f.build.ret(f.at());
    f.build.set_block(other);
    f.build.ret(f.at());
}

static void verifier_a_break_out_of_no_region_around_it(Fixture& f) {
    f.function("f", {}, NOTHING, IR_FORM_STRUCTURED);

    IrInstId first = f.build.begin_block(f.at());

    f.build.end();
    f.build.begin_block(f.at());
    f.build.break_to(first, f.at());
    f.build.end();
    f.build.ret(f.at());
}

static void verifier_a_continue_of_a_block(Fixture& f) {
    f.function("f", {}, NOTHING, IR_FORM_STRUCTURED);

    IrInstId region = f.build.begin_block(f.at());

    f.build.continue_to(region, f.at());
    f.build.end();
    f.build.ret(f.at());
}

static void verifier_operands_of_two_widths(Fixture& f) {
    f.function("f", {}, NOTHING, IR_FORM_STRUCTURED);
    IrValueId narrow = f.number(f.b32, 1);
    IrValueId wide = f.number(f.b64, 2);

    f.build.binary(IR_ADD, f.b32, narrow, wide, f.at());
    f.build.ret(f.at());
}

static void verifier_a_float_operation_on_whole_numbers(Fixture& f) {
    f.function("f", {}, NOTHING, IR_FORM_STRUCTURED);
    IrValueId one = f.number(f.b32, 1);
    IrValueId two = f.number(f.b32, 2);

    f.build.binary(IR_FADD, f.b32, one, two, f.at());
    f.build.ret(f.at());
}

static void verifier_an_ordered_comparison_of_floats(Fixture& f) {
    f.function("f", {f.f32}, NOTHING, IR_FORM_STRUCTURED);
    f.build.binary(IR_SLT, f.f32, f.build.parameter(0), f.build.parameter(0),
                   f.at());
    f.build.ret(f.at());
}

static void verifier_a_widening_that_narrows(Fixture& f) {
    f.function("f", {f.b64}, NOTHING, IR_FORM_STRUCTURED);
    f.build.convert(IR_ZEXT, f.b32, f.build.parameter(0), f.at());
    f.build.ret(f.at());
}

static void verifier_a_conversion_of_the_wrong_kind(Fixture& f) {
    f.function("f", {f.f32}, NOTHING, IR_FORM_STRUCTURED);
    f.build.convert(IR_SITOFP, f.f64, f.build.parameter(0), f.at());
    f.build.ret(f.at());
}

static void verifier_a_select_on_a_number(Fixture& f) {
    f.function("f", {f.b32}, NOTHING, IR_FORM_STRUCTURED);

    IrValueId x = f.build.parameter(0);

    f.build.select(f.b32, x, x, x, f.at());
    f.build.ret(f.at());
}

static void verifier_a_constant_pointer(Fixture& f) {
    f.function("f", {}, NOTHING, IR_FORM_STRUCTURED);
    f.number(f.ptr, 0);
    f.build.ret(f.at());
}

static void verifier_a_load_of_an_aggregate(Fixture& f) {
    IrTypeId point;
    IrTypeId square;

    shapes(f, point, square);
    f.function("f", {}, NOTHING, IR_FORM_STRUCTURED);
    f.build.load(point, f.build.slot(point, f.at()), f.at());
    f.build.ret(f.at());
}

static void verifier_a_copy_of_a_scalar(Fixture& f) {
    f.function("f", {}, NOTHING, IR_FORM_STRUCTURED);

    IrValueId cell = f.build.slot(f.b32, f.at());

    f.build.copy(f.b32, cell, cell, f.at());
    f.build.ret(f.at());
}

static void verifier_a_field_that_is_not_there(Fixture& f) {
    IrTypeId point;
    IrTypeId square;

    shapes(f, point, square);
    f.function("f", {}, NOTHING, IR_FORM_STRUCTURED);
    f.build.field(point, f.build.slot(point, f.at()), 2, f.at());
    f.build.ret(f.at());
}

static void verifier_an_index_by_a_float(Fixture& f) {
    f.function("f", {f.f32}, NOTHING, IR_FORM_STRUCTURED);
    f.build.index(f.b32, f.build.slot(f.types->array(f.b32, 2), f.at()),
                  f.build.parameter(0), f.at());
    f.build.ret(f.at());
}

static void verifier_an_upcast_to_what_is_not_a_base(Fixture& f) {
    IrTypeId point;
    IrTypeId square;

    shapes(f, point, square);
    f.function("f", {f.ptr}, NOTHING, IR_FORM_STRUCTURED);
    f.build.upcast(square, point, f.build.parameter(0), f.at());
    f.build.ret(f.at());
}

static void verifier_a_call_with_too_many_arguments(Fixture& f) {
    IrObjectId callee = f.function("callee", {}, NOTHING, IR_FORM_STRUCTURED);

    f.build.ret(f.at());
    f.function("f", {}, NOTHING, IR_FORM_STRUCTURED);
    f.build.call(callee, {f.number(f.b32, 1)}, f.at());
    f.build.ret(f.at());
}

static void verifier_a_call_with_an_argument_of_the_wrong_type(Fixture& f) {
    IrObjectId callee = f.function("callee", {f.b64}, NOTHING,
                                   IR_FORM_STRUCTURED);

    f.build.ret(f.at());
    f.function("f", {}, NOTHING, IR_FORM_STRUCTURED);
    f.build.call(callee, {f.number(f.b32, 1)}, f.at());
    f.build.ret(f.at());
}

static void verifier_a_call_to_data(Fixture& f) {
    IrObjectId data = f.program.add_zeroed("data", f.b32);

    f.function("f", {}, NOTHING, IR_FORM_STRUCTURED);
    f.build.call(data, {}, f.at());
    f.build.ret(f.at());
}

static void verifier_a_vcall_on_a_struct(Fixture& f) {
    IrTypeId point;
    IrTypeId square;

    shapes(f, point, square);
    f.function("f", {f.ptr}, NOTHING, IR_FORM_STRUCTURED);
    f.build.vcall(point, 0, 0, {f.build.parameter(0)}, f.at());
    f.build.ret(f.at());
}

static void verifier_a_vcall_of_a_slot_that_is_not_there(Fixture& f) {
    IrTypeId point;
    IrTypeId square;

    shapes(f, point, square);
    f.function("f", {f.ptr}, NOTHING, IR_FORM_STRUCTURED);
    f.build.vcall(square, 5, 0, {f.build.parameter(0)}, f.at());
    f.build.ret(f.at());
}

static void verifier_a_vcall_with_another_signature(Fixture& f) {
    IrTypeId point;
    IrTypeId square;

    shapes(f, point, square);
    f.function("f", {f.ptr}, NOTHING, IR_FORM_STRUCTURED);
    f.build.vcall(square, 1, f.program.signature({f.ptr}, f.b64),
                  {f.build.parameter(0)}, f.at());
    f.build.ret(f.at());
}

static void verifier_a_value_used_before_it_is_made(Fixture& f) {
    f.function("f", {}, NOTHING, IR_FORM_STRUCTURED);

    IrValueId later = {2};

    f.build.binary(IR_ADD, f.b32, later, later, f.at());
    f.number(f.b32, 1);
    f.build.ret(f.at());
}

static void verifier_a_value_used_outside_its_region(Fixture& f) {
    f.function("f", {}, f.b32, IR_FORM_STRUCTURED);
    f.build.begin_block(f.at());

    IrValueId inside = f.number(f.b32, 1);

    f.build.end();
    f.build.ret(f.b32, inside, f.at());
}

static void verifier_a_value_from_a_branch_not_taken(Fixture& f) {
    f.function("f", {f.boolean}, f.b32, IR_FORM_CFG);

    IrBlockId left = f.build.new_block({});
    IrBlockId right = f.build.new_block({});
    IrBlockId join = f.build.new_block({});

    f.build.cond_br(f.build.parameter(0), left, right, f.at());
    f.build.set_block(left);

    IrValueId made = f.number(f.b32, 1);

    f.build.br(join, {}, f.at());
    f.build.set_block(right);
    f.build.br(join, {}, f.at());
    f.build.set_block(join);
    f.build.ret(f.b32, made, f.at());
}

static void verifier_a_return_with_nothing(Fixture& f) {
    f.function("f", {}, f.b32, IR_FORM_STRUCTURED);
    f.build.ret(f.at());
}

static void verifier_a_return_of_the_wrong_type(Fixture& f) {
    f.function("f", {}, f.b32, IR_FORM_STRUCTURED);
    f.build.ret(f.b64, f.number(f.b64, 1), f.at());
}

static void verifier_an_abort_with_a_writable_message(Fixture& f) {
    IrObjectId message = f.program.add_zeroed("message", f.b8);

    f.function("f", {}, NOTHING, IR_FORM_STRUCTURED);
    f.build.abort(message, f.at());
}

static void verifier_an_instruction_from_nowhere(Fixture& f) {
    IrPosition nowhere = {0, IR_NO_OFFSET};

    f.positions = true;
    f.function("f", {}, NOTHING, IR_FORM_STRUCTURED);
    f.build.fence(nowhere);
    f.build.ret(f.at());
}

static void verifier_a_real_time_loop_with_no_bound(Fixture& f) {
    IrObjectId made = f.function("f", {}, NOTHING, IR_FORM_STRUCTURED);

    f.program.get_function(made).real_time = true;
    f.build.begin_loop(8, f.at());
    f.build.end();
    f.build.begin_loop(IR_NO_BOUND, f.at());
    f.build.end();
    f.build.ret(f.at());
}

// ---- the interpreter (stage 3): each program flattened, then run ----

static void run_prints_a_number(Fixture& f) {
    f.need_print();
    f.function("main", {}, f.b32, IR_FORM_STRUCTURED);

    IrValueId big = f.number(f.b64, 1234567890123ull);
    IrValueId small = f.number(f.b64, 0);
    IrValueId below = f.number(f.b64, (u64) -42);

    f.print(big);
    f.print(small);
    f.print(below);
    f.build.ret(f.b32, f.number(f.b32, 7), f.at());
}

// questions 1, 2 and 3, each as Haard decided it
static void run_whole_number_semantics(Fixture& f) {
    f.need_print();
    f.function("main", {}, f.b32, IR_FORM_STRUCTURED);

    IrValueId most = f.number(f.b32, 0x7fffffff);
    IrValueId one = f.number(f.b32, 1);
    IrValueId least = f.number(f.b32, 0x80000000);
    IrValueId minus_one = f.number(f.b32, 0xffffffff);
    IrValueId forty = f.number(f.b32, 40);
    IrValueId seven = f.number(f.b32, 7);
    IrValueId minus_seven = f.number(f.b32, (u32) -7);
    IrValueId two = f.number(f.b32, 2);

    // 1a: overflow wraps
    f.print_b32(f.build.binary(IR_ADD, f.b32, most, one, f.at()));
    // 3a: MIN / -1 is MIN, MIN % -1 is 0, division truncates
    f.print_b32(f.build.binary(IR_SDIV, f.b32, least, minus_one, f.at()));
    f.print_b32(f.build.binary(IR_SREM, f.b32, least, minus_one, f.at()));
    f.print_b32(f.build.binary(IR_SDIV, f.b32, minus_seven, two, f.at()));
    f.print_b32(f.build.binary(IR_SREM, f.b32, minus_seven, two, f.at()));
    f.print_b32(f.build.binary(IR_UDIV, f.b32, minus_one, seven, f.at()));
    // 2b: a count of the width or more gives the arithmetic result
    f.print_b32(f.build.binary(IR_SHL, f.b32, seven, forty, f.at()));
    f.print_b32(f.build.binary(IR_LSHR, f.b32, minus_seven, forty, f.at()));
    f.print_b32(f.build.binary(IR_ASHR, f.b32, minus_seven, forty, f.at()));
    f.print_b32(f.build.binary(IR_ASHR, f.b32, minus_seven, one, f.at()));
    f.print_b32(f.build.binary(IR_SHL, f.b32, seven, minus_one, f.at()));
    // the high halves of a product
    f.print_b32(f.build.binary(IR_SMULHI, f.b32, minus_seven, most, f.at()));
    f.print_b32(f.build.binary(IR_UMULHI, f.b32, minus_one, minus_one, f.at()));

    IrValueId huge = f.number(f.b64, 0x8000000000000000ull);
    IrValueId four = f.number(f.b64, 4);
    IrValueId all = f.number(f.b64, 0xffffffffffffffffull);

    f.print(f.build.binary(IR_UMULHI, f.b64, huge, four, f.at()));
    f.print(f.build.binary(IR_SMULHI, f.b64, all, all, f.at()));
    f.print(f.build.binary(IR_SMULHI, f.b64, huge, four, f.at()));
    // a byte is a value too: 200 + 100 wraps at 8 bits
    IrValueId sum = f.build.binary(IR_ADD, f.b8, f.number(f.b8, 200),
                                   f.number(f.b8, 100), f.at());

    f.print(f.build.convert(IR_ZEXT, f.b64, sum, f.at()));
    f.print(f.build.convert(IR_SEXT, f.b64, f.number(f.b8, 200), f.at()));

    // MIN / -1 at 64 bits, where the host's own division would trap
    IrValueId minus = f.number(f.b64, 0xffffffffffffffffull);

    f.print(f.build.binary(IR_SDIV, f.b64, huge, minus, f.at()));
    f.print(f.build.binary(IR_SREM, f.b64, huge, minus, f.at()));
    f.build.ret(f.b32, f.number(f.b32, 0), f.at());
}

static void run_a_division_by_zero_aborts(Fixture& f) {
    f.function("main", {}, f.b32, IR_FORM_STRUCTURED);

    IrValueId ten = f.number(f.b32, 10);
    IrValueId zero = f.number(f.b32, 0);

    f.build.ret(f.b32, f.build.binary(IR_SDIV, f.b32, ten, zero, f.at()),
                f.at());
}

static void run_floats(Fixture& f) {
    f.need_print();
    f.function("main", {}, f.b32, IR_FORM_STRUCTURED);

    double a = 3.75;
    double b = -1.5;
    u64 bits_a = 0;
    u64 bits_b = 0;

    std::memcpy(&bits_a, &a, sizeof(a));
    std::memcpy(&bits_b, &b, sizeof(b));

    IrValueId x = f.number(f.f64, bits_a);
    IrValueId y = f.number(f.f64, bits_b);
    IrValueId product = f.build.binary(IR_FMUL, f.f64, x, y, f.at());
    IrValueId narrow = f.build.convert(IR_FPTRUNC, f.f32, x, f.at());
    IrValueId back = f.build.convert(IR_FPEXT, f.f64, narrow, f.at());

    // truncated toward zero: -5.625 is -5
    f.print(f.build.convert(IR_FPTOSI, f.b64, product, f.at()));
    f.print(f.build.convert(IR_FPTOSI, f.b64, back, f.at()));
    f.print(f.build.convert(IR_ZEXT, f.b64,
                            f.build.binary(IR_FLT, f.f64, y, x, f.at()),
                            f.at()));

    IrValueId twelve = f.number(f.b64, 12);
    IrValueId real = f.build.convert(IR_SITOFP, f.f64, twelve, f.at());

    f.print(f.build.convert(IR_FPTOSI, f.b64,
                            f.build.binary(IR_FDIV, f.f64, real, y, f.at()),
                            f.at()));
    f.build.ret(f.b32, f.number(f.b32, 0), f.at());
}

static void run_a_float_too_big_for_its_integer(Fixture& f) {
    f.function("main", {}, f.b32, IR_FORM_STRUCTURED);

    double big = 1e20;
    u64 bits = 0;

    std::memcpy(&bits, &big, sizeof(big));
    f.build.ret(f.b32,
                f.build.convert(IR_FPTOSI, f.b32, f.number(f.f64, bits),
                                f.at()),
                f.at());
}

// a struct in a slot, a copy, an array walked by a loop with its bound
static void run_memory_and_a_loop(Fixture& f) {
    IrTypeId point;
    IrTypeId square;

    shapes(f, point, square);
    f.need_print();
    f.function("main", {}, f.b32, IR_FORM_STRUCTURED);

    IrValueId here = f.build.slot(point, f.at());
    IrValueId there = f.build.slot(point, f.at());

    f.build.store(f.b32, f.number(f.b32, 3), f.build.field(point, here, 0,
                                                           f.at()),
                  f.at());
    f.build.store(f.b32, f.number(f.b32, 4), f.build.field(point, here, 1,
                                                           f.at()),
                  f.at());
    f.build.copy(point, there, here, f.at());
    f.print_b32(f.build.load(f.b32, f.build.field(point, there, 1, f.at()),
                             f.at()));

    IrValueId row = f.build.slot(f.types->array(f.b32, 10), f.at());
    IrValueId i = f.build.slot(f.b64, f.at());
    IrValueId total = f.build.slot(f.b32, f.at());
    IrValueId ten = f.number(f.b64, 10);
    IrValueId one = f.number(f.b64, 1);

    f.build.store(f.b64, f.number(f.b64, 0), i, f.at());
    f.build.store(f.b32, f.number(f.b32, 0), total, f.at());

    IrInstId fill = f.build.begin_loop(10, f.at());
    IrValueId now = f.build.load(f.b64, i, f.at());

    f.build.begin_if(f.build.binary(IR_UGE, f.b64, now, ten, f.at()), f.at());
    f.build.break_to(fill, f.at());
    f.build.end();

    IrValueId squared = f.build.binary(
        IR_MUL, f.b32, f.build.convert(IR_TRUNC, f.b32, now, f.at()),
        f.build.convert(IR_TRUNC, f.b32, now, f.at()), f.at());

    f.build.store(f.b32, squared, f.build.index(f.b32, row, now, f.at()),
                  f.at());

    IrValueId sum = f.build.binary(IR_ADD, f.b32,
                                   f.build.load(f.b32, total, f.at()),
                                   squared, f.at());

    f.build.store(f.b32, sum, total, f.at());
    f.build.store(f.b64, f.build.binary(IR_ADD, f.b64, now, one, f.at()), i,
                  f.at());
    f.build.end();

    f.print_b32(f.build.load(f.b32, total, f.at()));
    f.build.ret(f.b32, f.build.load(f.b32, total, f.at()), f.at());
}

static void run_a_loop_past_its_bound(Fixture& f) {
    f.function("main", {}, f.b32, IR_FORM_STRUCTURED);

    IrValueId i = f.build.slot(f.b32, f.at());
    IrValueId five = f.number(f.b32, 5);
    IrValueId one = f.number(f.b32, 1);

    f.build.store(f.b32, f.number(f.b32, 0), i, f.at());

    // it goes round five times and its bound says four
    IrInstId loop = f.build.begin_loop(4, f.at());
    IrValueId now = f.build.load(f.b32, i, f.at());

    f.build.begin_if(f.build.binary(IR_SGE, f.b32, now, five, f.at()), f.at());
    f.build.break_to(loop, f.at());
    f.build.end();
    f.build.store(f.b32, f.build.binary(IR_ADD, f.b32, now, one, f.at()), i,
                  f.at());
    f.build.end();
    f.build.ret(f.b32, f.build.load(f.b32, i, f.at()), f.at());
}

// a loop entered again starts counting again: the inner one goes round three
// times per entry, nine in all, under a bound of three
static void run_a_nested_loop_counts_per_entry(Fixture& f) {
    f.function("main", {}, f.b32, IR_FORM_STRUCTURED);

    IrValueId i = f.build.slot(f.b32, f.at());
    IrValueId j = f.build.slot(f.b32, f.at());
    IrValueId total = f.build.slot(f.b32, f.at());
    IrValueId three = f.number(f.b32, 3);
    IrValueId one = f.number(f.b32, 1);
    IrValueId zero = f.number(f.b32, 0);

    f.build.store(f.b32, zero, i, f.at());
    f.build.store(f.b32, zero, total, f.at());

    IrInstId outer = f.build.begin_loop(3, f.at());
    IrValueId now = f.build.load(f.b32, i, f.at());

    f.build.begin_if(f.build.binary(IR_SGE, f.b32, now, three, f.at()), f.at());
    f.build.break_to(outer, f.at());
    f.build.end();
    f.build.store(f.b32, zero, j, f.at());

    IrInstId inner = f.build.begin_loop(3, f.at());
    IrValueId inside = f.build.load(f.b32, j, f.at());

    f.build.begin_if(f.build.binary(IR_SGE, f.b32, inside, three, f.at()),
                     f.at());
    f.build.break_to(inner, f.at());
    f.build.end();
    f.build.store(f.b32, f.build.binary(IR_ADD, f.b32,
                                        f.build.load(f.b32, total, f.at()),
                                        one, f.at()),
                  total, f.at());
    f.build.store(f.b32, f.build.binary(IR_ADD, f.b32, inside, one, f.at()),
                  j, f.at());
    f.build.end();
    f.build.store(f.b32, f.build.binary(IR_ADD, f.b32, now, one, f.at()), i,
                  f.at());
    f.build.end();
    f.build.ret(f.b32, f.build.load(f.b32, total, f.at()), f.at());
}

// a 'continue' of the outer loop from inside the inner one is a turn of the
// outer loop: it goes round five times this way, under a bound of two, and
// that has to be seen through the inner loop it jumps out of
static void run_a_continue_of_the_outer_loop_is_its_turn(Fixture& f) {
    f.function("main", {}, f.b32, IR_FORM_STRUCTURED);

    IrValueId i = f.build.slot(f.b32, f.at());
    IrValueId five = f.number(f.b32, 5);
    IrValueId one = f.number(f.b32, 1);

    f.build.store(f.b32, f.number(f.b32, 0), i, f.at());

    IrInstId outer = f.build.begin_loop(2, f.at());
    IrValueId now = f.build.load(f.b32, i, f.at());

    f.build.begin_if(f.build.binary(IR_SGE, f.b32, now, five, f.at()), f.at());
    f.build.break_to(outer, f.at());
    f.build.end();
    f.build.begin_loop(1, f.at());
    f.build.store(f.b32, f.build.binary(IR_ADD, f.b32, now, one, f.at()), i,
                  f.at());
    f.build.continue_to(outer, f.at());
    f.build.end();
    f.build.end();
    f.build.ret(f.b32, f.build.load(f.b32, i, f.at()), f.at());
}

// an 'if' whose two sides both return has nothing after it: flatten drops
// the empty block it would have made there, which nothing jumps to
static void run_an_if_whose_two_sides_return(Fixture& f) {
    IrObjectId sign = f.function("sign", {f.b32}, f.b32, IR_FORM_STRUCTURED);
    IrValueId zero = f.number(f.b32, 0);

    f.build.begin_if(f.build.binary(IR_SLT, f.b32, f.build.parameter(0), zero,
                                    f.at()),
                     f.at());
    f.build.ret(f.b32, f.number(f.b32, 1), f.at());
    f.build.begin_else();
    f.build.ret(f.b32, f.number(f.b32, 2), f.at());
    f.build.end();

    f.function("main", {}, f.b32, IR_FORM_STRUCTURED);

    IrValueId below = f.number(f.b32, (u32) -3);

    f.build.ret(f.b32, f.build.call(sign, {below}, f.at()), f.at());
}

static void run_calls_a_recursion_and_a_pointer(Fixture& f) {
    IrObjectId factorial = f.function("factorial", {f.b64}, f.b64,
                                      IR_FORM_STRUCTURED);
    IrValueId n = f.build.parameter(0);
    IrValueId one = f.number(f.b64, 1);

    f.build.begin_if(f.build.binary(IR_ULE, f.b64, n, one, f.at()), f.at());
    f.build.ret(f.b64, one, f.at());
    f.build.end();

    IrValueId below = f.build.call(
        factorial, {f.build.binary(IR_SUB, f.b64, n, one, f.at())}, f.at());

    f.build.ret(f.b64, f.build.binary(IR_MUL, f.b64, n, below, f.at()),
                f.at());

    f.need_print();
    f.function("main", {}, f.b32, IR_FORM_STRUCTURED);

    IrValueId twenty = f.number(f.b64, 20);

    f.print(f.build.call(factorial, {twenty}, f.at()));

    IrValueId through = f.build.address(factorial, f.at());
    IrValueId five = f.number(f.b64, 5);

    f.print(f.build.call_indirect(f.program.signature({f.b64}, f.b64),
                                  through, {five}, f.at()));
    f.build.ret(f.b32, f.number(f.b32, 0), f.at());
}

// question 83's vcall, through a vtable that is a datum of the program
// (question 81): the descriptor's place first, then a slot per method, each
// holding a function's address by a relocation. An object's vptr points at
// slot 0, and a Square answers 'area' with its own method
static void run_a_virtual_call(Fixture& f) {
    IrTypeId point;
    IrTypeId square;
    IrTypeId shape = shapes(f, point, square);

    IrObjectId shape_area = f.function("Shape.area", {f.ptr}, f.b32,
                                       IR_FORM_STRUCTURED);

    f.build.ret(f.b32, f.number(f.b32, 0), f.at());

    IrObjectId square_area = f.function("Square.area", {f.ptr}, f.b32,
                                        IR_FORM_STRUCTURED);
    IrValueId self = f.build.downcast(shape, square, f.build.parameter(0),
                                      f.at());
    IrValueId side = f.build.load(f.b32, f.build.field(square, self, 0,
                                                       f.at()),
                                  f.at());

    f.build.ret(f.b32, f.build.binary(IR_MUL, f.b32, side, side, f.at()),
                f.at());

    IrObjectId destroy = f.function("destroy", {f.ptr}, NOTHING,
                                    IR_FORM_STRUCTURED);

    f.build.ret(f.at());

    IrTypeId table_type = f.types->array(f.ptr, 3);
    IrObjectId shape_table = f.program.add_data(
        "Shape.vtable", table_type, std::vector<u8>(24, 0), true);
    IrObjectId square_table = f.program.add_data(
        "Square.vtable", table_type, std::vector<u8>(24, 0), true);

    f.program.add_relocation(shape_table, 8, destroy);
    f.program.add_relocation(shape_table, 16, shape_area);
    f.program.add_relocation(square_table, 8, destroy);
    f.program.add_relocation(square_table, 16, square_area);

    f.need_print();
    f.function("main", {}, f.b32, IR_FORM_STRUCTURED);

    IrValueId made = f.build.slot(square, f.at());
    IrValueId eight = f.number(f.b64, 1);
    IrValueId table = f.build.index(f.ptr, f.build.address(square_table,
                                                           f.at()),
                                    eight, f.at());

    // what construction will write: the vptr, then the fields
    f.build.store(f.ptr, table, made, f.at());
    f.build.store(f.b32, f.number(f.b32, 9),
                  f.build.field(square, made, 0, f.at()), f.at());

    IrValueId as_shape = f.build.upcast(square, shape, made, f.at());
    IrValueId area = f.build.vcall(shape, 1, f.program.signature({f.ptr},
                                                                 f.b32),
                                   {as_shape}, f.at());

    f.print_b32(area);
    f.build.ret(f.b32, area, f.at());
}

// a class derived from a struct has its vptr first and the struct after it
// (record 0065), so an upcast moves the pointer past the vptr: by 8
static void run_an_upcast_past_the_vptr(Fixture& f) {
    IrTypeId point;
    IrTypeId square;

    shapes(f, point, square);

    IrTypeId named = f.types->aggregate(IR_CLASS, "Named");
    IrField tag;

    tag.name = "tag";
    tag.type = f.b32;
    tag.candidate = 0;
    f.types->get_aggregate(named)->base = point;
    f.types->get_aggregate(named)->fields.push_back(tag);
    f.need_print();
    f.function("main", {}, f.b32, IR_FORM_STRUCTURED);

    IrValueId made = f.build.slot(named, f.at());
    IrValueId as_point = f.build.upcast(named, point, made, f.at());
    IrValueId from = f.build.convert(IR_PTRTOINT, f.b64, made, f.at());
    IrValueId to = f.build.convert(IR_PTRTOINT, f.b64, as_point, f.at());

    f.print(f.build.binary(IR_SUB, f.b64, to, from, f.at()));

    IrValueId back = f.build.downcast(point, named, as_point, f.at());

    f.print(f.build.convert(IR_ZEXT, f.b64,
                            f.build.binary(IR_EQ, f.ptr, back, made, f.at()),
                            f.at()));
    f.build.ret(f.b32, f.number(f.b32, 0), f.at());
}

static void run_a_slot_used_after_its_frame(Fixture& f) {
    IrObjectId leak = f.function("leak", {}, f.ptr, IR_FORM_STRUCTURED);

    f.build.ret(f.ptr, f.build.slot(f.b32, f.at()), f.at());
    f.function("main", {}, f.b32, IR_FORM_STRUCTURED);
    f.build.ret(f.b32, f.build.load(f.b32, f.build.call(leak, {}, f.at()),
                                    f.at()),
                f.at());
}

static void run_an_index_past_the_end(Fixture& f) {
    f.function("main", {}, f.b32, IR_FORM_STRUCTURED);

    IrValueId row = f.build.slot(f.types->array(f.b32, 4), f.at());
    IrValueId four = f.number(f.b64, 4);

    f.build.ret(f.b32, f.build.load(f.b32, f.build.index(f.b32, row, four,
                                                         f.at()),
                                    f.at()),
                f.at());
}

static void run_a_write_to_read_only_data(Fixture& f) {
    IrObjectId text = f.program.add_text("hello", "hello");

    f.function("main", {}, f.b32, IR_FORM_STRUCTURED);
    f.build.store(f.b8, f.number(f.b8, 'j'), f.build.address(text, f.at()),
                  f.at());
    f.build.ret(f.b32, f.number(f.b32, 0), f.at());
}

static void run_a_load_through_null(Fixture& f) {
    f.function("main", {}, f.b32, IR_FORM_STRUCTURED);
    f.build.ret(f.b32, f.build.load(f.b32, f.build.null(f.at()), f.at()),
                f.at());
}

// question 10: a slot nobody wrote holds the interpreter's fixed pattern, the
// same on every run, and nothing a program may count on
static void run_memory_nobody_wrote(Fixture& f) {
    f.need_print();
    f.function("main", {}, f.b32, IR_FORM_STRUCTURED);

    IrValueId fresh = f.build.slot(f.b64, f.at());

    f.print(f.build.load(f.b64, fresh, f.at()));
    f.build.ret(f.b32, f.number(f.b32, 0), f.at());
}

static void run_an_abort_with_its_message(Fixture& f) {
    IrObjectId message = f.program.add_text("unwrapped", "unwrap of None");

    f.function("main", {}, f.b32, IR_FORM_STRUCTURED);
    f.build.abort(message, f.at());
}

// the same bytes read on a big-endian target: the first byte of 0x01020304
// is 1 on the processor and 4 on x86-64, its first two 258 and 772
static void run_big_endian_bytes(Fixture& f) {
    f.target = "pret";
    f.need_print();
    f.function("main", {}, f.b32, IR_FORM_STRUCTURED);

    IrValueId word = f.build.slot(f.b32, f.at());

    f.build.store(f.b32, f.number(f.b32, 0x01020304), word, f.at());
    f.print(f.build.convert(IR_ZEXT, f.b64, f.build.load(f.b16, word, f.at()),
                            f.at()));
    f.build.ret(f.b32, f.build.convert(IR_ZEXT, f.b32,
                                       f.build.load(f.b8, word, f.at()),
                                       f.at()),
                f.at());
}

static void run_little_endian_bytes(Fixture& f) {
    f.need_print();
    f.function("main", {}, f.b32, IR_FORM_STRUCTURED);

    IrValueId word = f.build.slot(f.b32, f.at());

    f.build.store(f.b32, f.number(f.b32, 0x01020304), word, f.at());
    f.print(f.build.convert(IR_ZEXT, f.b64, f.build.load(f.b16, word, f.at()),
                            f.at()));
    f.build.ret(f.b32, f.build.convert(IR_ZEXT, f.b32,
                                       f.build.load(f.b8, word, f.at()),
                                       f.at()),
                f.at());
}

struct Case {
    const char* name;
    void (*build)(Fixture& f);
};

static const Case CASES[] = {
    {"every_integer_operation", every_integer_operation},
    {"floats_and_conversions", floats_and_conversions},
    {"constants_and_objects", constants_and_objects},
    {"memory_and_aggregates", memory_and_aggregates},
    {"calls", calls},
    {"barriers", barriers},
    {"regions_nest", regions_nest},
    {"a_cfg_with_block_parameters", a_cfg_with_block_parameters},
    {"abort_and_positions", abort_and_positions},
    {"every_opcode_has_its_effects", every_opcode_has_its_effects},
    {"run_prints_a_number", run_prints_a_number},
    {"run_whole_number_semantics", run_whole_number_semantics},
    {"run_a_division_by_zero_aborts", run_a_division_by_zero_aborts},
    {"run_floats", run_floats},
    {"run_a_float_too_big_for_its_integer",
     run_a_float_too_big_for_its_integer},
    {"run_memory_and_a_loop", run_memory_and_a_loop},
    {"run_a_loop_past_its_bound", run_a_loop_past_its_bound},
    {"run_a_nested_loop_counts_per_entry", run_a_nested_loop_counts_per_entry},
    {"run_a_continue_of_the_outer_loop_is_its_turn",
     run_a_continue_of_the_outer_loop_is_its_turn},
    {"run_an_if_whose_two_sides_return", run_an_if_whose_two_sides_return},
    {"run_calls_a_recursion_and_a_pointer",
     run_calls_a_recursion_and_a_pointer},
    {"run_a_virtual_call", run_a_virtual_call},
    {"run_an_upcast_past_the_vptr", run_an_upcast_past_the_vptr},
    {"run_a_slot_used_after_its_frame", run_a_slot_used_after_its_frame},
    {"run_an_index_past_the_end", run_an_index_past_the_end},
    {"run_a_write_to_read_only_data", run_a_write_to_read_only_data},
    {"run_a_load_through_null", run_a_load_through_null},
    {"run_memory_nobody_wrote", run_memory_nobody_wrote},
    {"run_an_abort_with_its_message", run_an_abort_with_its_message},
    {"run_big_endian_bytes", run_big_endian_bytes},
    {"run_little_endian_bytes", run_little_endian_bytes},
    {"verifier_a_jump_in_a_structured_function",
     verifier_a_jump_in_a_structured_function},
    {"verifier_a_region_in_a_cfg", verifier_a_region_in_a_cfg},
    {"verifier_something_after_a_terminator",
     verifier_something_after_a_terminator},
    {"verifier_a_block_that_does_not_end", verifier_a_block_that_does_not_end},
    {"verifier_an_empty_block", verifier_an_empty_block},
    {"verifier_a_jump_to_the_entry", verifier_a_jump_to_the_entry},
    {"verifier_a_jump_out_of_the_function",
     verifier_a_jump_out_of_the_function},
    {"verifier_too_few_block_arguments", verifier_too_few_block_arguments},
    {"verifier_a_block_argument_of_the_wrong_type",
     verifier_a_block_argument_of_the_wrong_type},
    {"verifier_cond_br_to_a_block_with_parameters",
     verifier_cond_br_to_a_block_with_parameters},
    {"verifier_a_break_out_of_no_region_around_it",
     verifier_a_break_out_of_no_region_around_it},
    {"verifier_a_continue_of_a_block", verifier_a_continue_of_a_block},
    {"verifier_operands_of_two_widths", verifier_operands_of_two_widths},
    {"verifier_a_float_operation_on_whole_numbers",
     verifier_a_float_operation_on_whole_numbers},
    {"verifier_an_ordered_comparison_of_floats",
     verifier_an_ordered_comparison_of_floats},
    {"verifier_a_widening_that_narrows", verifier_a_widening_that_narrows},
    {"verifier_a_conversion_of_the_wrong_kind",
     verifier_a_conversion_of_the_wrong_kind},
    {"verifier_a_select_on_a_number", verifier_a_select_on_a_number},
    {"verifier_a_constant_pointer", verifier_a_constant_pointer},
    {"verifier_a_load_of_an_aggregate", verifier_a_load_of_an_aggregate},
    {"verifier_a_copy_of_a_scalar", verifier_a_copy_of_a_scalar},
    {"verifier_a_field_that_is_not_there", verifier_a_field_that_is_not_there},
    {"verifier_an_index_by_a_float", verifier_an_index_by_a_float},
    {"verifier_an_upcast_to_what_is_not_a_base",
     verifier_an_upcast_to_what_is_not_a_base},
    {"verifier_a_call_with_too_many_arguments",
     verifier_a_call_with_too_many_arguments},
    {"verifier_a_call_with_an_argument_of_the_wrong_type",
     verifier_a_call_with_an_argument_of_the_wrong_type},
    {"verifier_a_call_to_data", verifier_a_call_to_data},
    {"verifier_a_vcall_on_a_struct", verifier_a_vcall_on_a_struct},
    {"verifier_a_vcall_of_a_slot_that_is_not_there",
     verifier_a_vcall_of_a_slot_that_is_not_there},
    {"verifier_a_vcall_with_another_signature",
     verifier_a_vcall_with_another_signature},
    {"verifier_a_value_used_before_it_is_made",
     verifier_a_value_used_before_it_is_made},
    {"verifier_a_value_used_outside_its_region",
     verifier_a_value_used_outside_its_region},
    {"verifier_a_value_from_a_branch_not_taken",
     verifier_a_value_from_a_branch_not_taken},
    {"verifier_a_return_with_nothing", verifier_a_return_with_nothing},
    {"verifier_a_return_of_the_wrong_type",
     verifier_a_return_of_the_wrong_type},
    {"verifier_an_abort_with_a_writable_message",
     verifier_an_abort_with_a_writable_message},
    {"verifier_an_instruction_from_nowhere",
     verifier_an_instruction_from_nowhere},
    {"verifier_a_real_time_loop_with_no_bound",
     verifier_a_real_time_loop_with_no_bound},
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: cases --list | cases <name>\n";
        return 2;
    }

    if (std::strcmp(argv[1], "--list") == 0) {
        for (const Case& one : CASES) {
            std::cout << one.name << "\n";
        }

        return 0;
    }

    for (const Case& one : CASES) {
        if (std::strcmp(argv[1], one.name) != 0) {
            continue;
        }

        Fixture f;
        IrVerifier verifier;
        IrPrinter printer;

        one.build(f);
        verifier.set_program(&f.program);
        printer.set_program(&f.program);
        printer.set_show_positions(f.positions);

        bool good = verifier.verify();

        if (!good) {
            printer.set_mark(verifier.get_errors()[0].function,
                             verifier.get_errors()[0].inst);
        }

        printer.print(std::cout);
        std::cout << "\nverifier:" << (good ? " ok" : "") << "\n";

        for (const IrVerifierError& error : verifier.get_errors()) {
            std::cout << "    " << error.message << "\n";
        }

        if (!good) {
            return 0;
        }

        // stage 3: every structured function to a CFG, verified again
        IrFlattener flattener;
        IrObjectId main_function = {0};
        bool flattened = false;

        flattener.set_program(&f.program);

        for (u32 i = 1; i < f.program.get_object_count(); i++) {
            IrObject& object = f.program.get_object({i});

            if (object.kind != IR_OBJECT_FUNCTION || object.native) {
                continue;
            }

            if (object.name == "main") {
                main_function.index = i;
            }

            if (f.program.get_function({i}).form == IR_FORM_STRUCTURED) {
                flattener.flatten({i});
                flattened = true;
            }
        }

        if (flattened) {
            good = verifier.verify();
            std::cout << "\nafter flatten:\n\n";
            printer.set_mark(IrObjectId{0}, IrInstId{0});
            printer.print(std::cout);
            std::cout << "\nverifier:" << (good ? " ok" : "") << "\n";

            for (const IrVerifierError& error : verifier.get_errors()) {
                std::cout << "    " << error.message << "\n";
            }
        }

        if (!good || main_function.index == 0) {
            return 0;
        }

        Target target;
        IrInterpreter interpreter;
        std::ostringstream output;

        find_target(f.target, target);
        interpreter.set_program(&f.program);
        interpreter.set_target(&target);
        interpreter.set_output(&output);

        IrRunResult ran = interpreter.run(main_function, {});

        std::cout << "\nrun on " << f.target << ":\n" << output.str();

        if (ran.kind == IR_RUN_EXIT) {
            // what the operating system keeps of what main gives back
            std::cout << "exit " << (ran.value & 0xff) << "\n";
        } else {
            std::cout << (ran.kind == IR_RUN_ABORT ? "aborted" : "stopped")
                      << " in @" << f.program.get_object(ran.function).name
                      << " at #" << ran.inst.index << ": " << ran.message
                      << "\n";
        }

        return 0;
    }

    std::cerr << "no case called '" << argv[1] << "'\n";
    return 2;
}
