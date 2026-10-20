// The IR's own cases, built by hand with the IrBuilder (record 0077, stage
// 2): there is no lowering yet, and question 88 asked for the verifier's
// cases to be IR broken on purpose, which only a builder can make.
//
//   cases --list      every case's name
//   cases <name>      the program printed, then what the verifier says
//
// A case whose name starts with 'verifier_' breaks one rule, and its golden
// holds the message and the program with the culprit marked '>>'. Every other
// case is well formed and its golden ends in 'verifier: ok'.
//
// One instruction per statement wherever the order matters: C++ leaves the
// order in which a call's arguments are evaluated unspecified, so two
// instructions made inside one argument list could come out either way.
#include <haard/ir/ir_builder.h>
#include <haard/ir/ir_printer.h>
#include <haard/ir/ir_verifier.h>

#include <cstring>
#include <iostream>

using namespace haard;

struct Fixture {
    IrProgram program;
    IrBuilder build;
    IrTypeTable* types;
    u32 offset;
    bool positions;

    IrTypeId b8, b16, b32, b64, boolean, f32, f64, ptr;

    Fixture() {
        build.set_program(&program);
        types = program.get_types();
        offset = 0;
        positions = false;
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
};

static const IrTypeId NOTHING = {0};

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

        return 0;
    }

    std::cerr << "no case called '" << argv[1] << "'\n";
    return 2;
}
