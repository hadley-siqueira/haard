#include <haard/lowering/function_lowerer.h>

#include <cstdlib>
#include <cstring>

using namespace haard;

FunctionLowerer::FunctionLowerer(IrLowerer* owner, IrProgram* program) {
    this->owner = owner;
    this->program = program;
    compilation = owner->get_compilation();
    module_index = 0;
    module = nullptr;
    result.index = 0;
    failed = false;
}

bool FunctionLowerer::lower(u32 module, u32 node, IrObjectId function) {
    AstQuery query;

    module_index = module;
    this->module = compilation->get_module(module);
    query.set_module(this->module);
    build.set_program(program);
    build.begin_function(function);
    result = program->get_signature(program->get_function(function).signature)
                 .result;

    // every parameter is copied into a slot on entry (question 82)
    std::vector<u32> parameters = query.get_params(node);

    for (u32 i = 0; i < parameters.size(); i++) {
        u32 candidate = this->module->get_symbols()->candidate_of(parameters[i]);
        IrTypeId type = program->get_signature(
                                   program->get_function(function).signature)
                            .parameters[i];
        IrValueId slot = build.entry_slot(type, at(parameters[i]));

        build.store(type, build.parameter(i), slot, at(parameters[i]));
        slots[candidate] = slot;
    }

    lower_block(query.get_block(node));

    // a function that gives nothing back may end without saying so
    if (!failed && !build.ended() && result.index == 0) {
        build.ret(at(node));
    }

    return !failed;
}

void FunctionLowerer::lower_block(u32 node) {
    if (node == 0) {
        return;
    }

    for (u32 child = child_of(node, 0); child != 0 && !failed;
         child = module->get_ast()->get_node(child)->get_sibling()) {
        // what follows a 'return', a 'break' or a 'continue' in the same
        // block never runs, and the IR has no place to put it
        if (build.ended()) {
            return;
        }

        lower_statement(child);
    }
}

void FunctionLowerer::lower_statement(u32 node) {
    switch (kind_of(node)) {
    case AST_PASS:
        return;

    // a block as a statement, which the sugar passes write: a foreach's
    // cursor and its loop
    case AST_BLOCK:
        lower_block(node);
        return;

    case AST_IF:
        lower_if(node);
        return;

    case AST_WHILE:
        lower_while(node);
        return;

    case AST_FOR:
        lower_for(node);
        return;

    case AST_LET_DECLARATION:
    case AST_CONST_DECLARATION:
        lower_binding(node);
        return;

    case AST_ASSIGNMENT:
        if (module->get_symbols()->candidate_of(node) != 0) {
            lower_declaring_assignment(node);
            return;
        }

        break;

    case AST_RETURN:
        lower_return(node);
        return;

    case AST_BREAK:
        build.break_to(loops.back().broken, at(node));
        return;

    case AST_CONTINUE:
        if (loops.back().continue_breaks) {
            build.break_to(loops.back().continued, at(node));
        } else {
            build.continue_to(loops.back().continued, at(node));
        }

        return;

    case AST_SWITCH:
        not_yet("a switch");
        return;

    default:
        break;
    }

    // an expression written for what it does
    value_of(node);
}

// 'elif' is an 'if' in the 'else' of the one before. The tree hangs every
// 'elif' and the 'else' on the 'if' itself, after its block
void FunctionLowerer::lower_if(u32 node) {
    u32 block = child_of(node, 1);

    lower_branches(child_of(node, 0), block,
                   block == 0 ? 0
                              : module->get_ast()->get_node(block)
                                    ->get_sibling(),
                   node);
}

void FunctionLowerer::lower_branches(u32 condition, u32 block, u32 next,
                                     u32 node) {
    IrValueId test = value_of(condition);

    if (failed) {
        return;
    }

    build.begin_if(test, at(node));
    lower_block(block);

    if (next != 0) {
        build.begin_else();

        if (kind_of(next) == AST_ELIF) {
            lower_branches(child_of(next, 0), child_of(next, 1),
                           module->get_ast()->get_node(next)->get_sibling(),
                           next);
        } else {
            lower_block(child_of(next, 0));
        }
    }

    build.end();
}

void FunctionLowerer::lower_while(u32 node) {
    IrInstId loop = build.begin_loop(IR_NO_BOUND, at(node));
    IrValueId test = value_of(child_of(node, 0));

    if (failed) {
        return;
    }

    build.begin_if(build.unary(IR_NOT, program->get_types()->scalar(IR_BOOL),
                               test, at(node)),
                   at(node));
    build.break_to(loop, at(node));
    build.end();

    Loop around = {loop, loop, false};

    loops.push_back(around);
    lower_block(child_of(node, 1));
    loops.pop_back();
    build.end();
}

void FunctionLowerer::lower_for(u32 node) {
    u32 head = 0;
    u32 condition = 0;
    u32 increment = 0;
    u32 block = 0;

    for (u32 part = child_of(node, 0); part != 0;
         part = module->get_ast()->get_node(part)->get_sibling()) {
        switch (kind_of(part)) {
        case AST_FOR_HEAD:
            head = part;
            break;

        case AST_FOR_CONDITION:
            condition = part;
            break;

        case AST_FOR_INCREMENT:
            increment = part;
            break;

        case AST_BLOCK:
            block = part;
            break;

        default:
            break;
        }
    }

    for (u32 one = head == 0 ? 0 : child_of(head, 0); one != 0 && !failed;
         one = module->get_ast()->get_node(one)->get_sibling()) {
        lower_statement(one);
    }

    IrInstId loop = build.begin_loop(IR_NO_BOUND, at(node));
    u32 test = condition == 0 ? 0 : child_of(condition, 0);

    if (test != 0) {
        IrValueId value = value_of(test);

        if (failed) {
            return;
        }

        build.begin_if(build.unary(IR_NOT,
                                   program->get_types()->scalar(IR_BOOL),
                                   value, at(node)),
                       at(node));
        build.break_to(loop, at(node));
        build.end();
    }

    IrInstId body = build.begin_block(at(node));
    Loop around = {loop, body, true};

    loops.push_back(around);
    lower_block(block);
    loops.pop_back();
    build.end();

    for (u32 one = increment == 0 ? 0 : child_of(increment, 0);
         one != 0 && !failed;
         one = module->get_ast()->get_node(one)->get_sibling()) {
        value_of(one);
    }

    build.end();
}

void FunctionLowerer::lower_binding(u32 node) {
    AstQuery query;

    query.set_module(module);

    // 'let a, b = 1, 2' is several bindings under one statement
    for (u32 binding = child_of(node, 0); binding != 0 && !failed;
         binding = module->get_ast()->get_node(binding)->get_sibling()) {
        u32 candidate = module->get_symbols()->candidate_of(binding);

        if (candidate == 0) {
            not_yet("a binding that takes a value apart");
            return;
        }

        u32 type = module->get_symbols()->get_candidate(candidate)->type;
        IrTypeId ir = ir_type(type);

        if (failed) {
            return;
        }

        IrValueId slot = build.entry_slot(ir, at(binding));
        u32 given = query.get_binding_expression(binding);

        slots[candidate] = slot;

        // record 0026: what is not given a value holds none
        if (given != 0) {
            IrValueId value = value_of(given);

            if (!failed) {
                build.store(ir, value, slot, at(binding));
            }
        }
    }
}

void FunctionLowerer::lower_declaring_assignment(u32 node) {
    u32 candidate = module->get_symbols()->candidate_of(node);
    u32 type = module->get_symbols()->get_candidate(candidate)->type;
    IrTypeId ir = ir_type(type);

    if (failed) {
        return;
    }

    IrValueId slot = build.entry_slot(ir, at(node));
    IrValueId value = value_of(child_of(node, 1));

    slots[candidate] = slot;

    if (!failed) {
        build.store(ir, value, slot, at(node));
    }
}

void FunctionLowerer::lower_return(u32 node) {
    u32 given = child_of(node, 0);

    if (given == 0) {
        build.ret(at(node));
        return;
    }

    IrValueId value = value_of(given);

    if (!failed) {
        build.ret(result, value, at(node));
    }
}

IrValueId FunctionLowerer::value_of(u32 node) {
    IrValueId none = {0};

    if (failed) {
        return none;
    }

    // record 0034: an operator a class overloads is a call to its method
    AstNodeKind kind = kind_of(node);

    if (kind != AST_CALL && kind != AST_IDENTIFIER
        && module->get_resolutions()->get(node)->candidate != 0) {
        not_yet("an operator a class overloads, or a construction");
        return none;
    }

    switch (kind) {
    case AST_INTEGER_LITERAL:
    case AST_FLOAT_LITERAL:
    case AST_CHAR_LITERAL:
    case AST_TRUE:
    case AST_FALSE:
        return literal(node);

    case AST_PARENTHESIS:
        return value_of(child_of(node, 0));

    case AST_IDENTIFIER: {
        IrValueId slot = place_of(node);
        IrTypeId type = ir_type(type_at(node));

        return failed ? none : build.load(type, slot, at(node));
    }

    case AST_CALL:
        return call(node);

    case AST_PLUS:
    case AST_MINUS:
    case AST_TIMES:
    case AST_DIVISION:
    case AST_MODULO:
    case AST_INTEGER_DIVISION:
    case AST_BITWISE_AND:
    case AST_BITWISE_OR:
    case AST_BITWISE_XOR:
    case AST_BITWISE_LEFT_SHIFT:
    case AST_BITWISE_RIGHT_SHIFT:
    case AST_BITWISE_UNSIGNED_RIGHT_SHIFT:
    case AST_POWER:
        return arithmetic(node);

    case AST_EQUAL:
    case AST_NOT_EQUAL:
    case AST_LESS_THAN:
    case AST_GREATER_THAN:
    case AST_LESS_THAN_OR_EQUAL:
    case AST_GREATER_THAN_OR_EQUAL:
        return comparison(node);

    case AST_LOGICAL_AND:
    case AST_LOGICAL_OR:
        return logical(node);

    case AST_LOGICAL_NOT:
    case AST_LOGICAL_NOT_OPERATOR: {
        IrValueId operand = value_of(child_of(node, 0));

        return failed ? none
                      : build.unary(IR_NOT,
                                    program->get_types()->scalar(IR_BOOL),
                                    operand, at(node));
    }

    case AST_UNARY_PLUS:
        return value_of(child_of(node, 0));

    case AST_UNARY_MINUS:
    case AST_BITWISE_NOT: {
        u32 type = type_at(node);
        IrValueId operand = value_of(child_of(node, 0));

        if (failed) {
            return none;
        }

        if (kind == AST_UNARY_MINUS && is_float_type(type)) {
            return build.unary(IR_FNEG, ir_type(type), operand, at(node));
        }

        if (!is_whole_type(type)) {
            not_yet("'" + text_of(node) + "' on this type");
            return none;
        }

        return build.unary(kind == AST_UNARY_MINUS ? IR_NEG : IR_NOT,
                           ir_type(type), operand, at(node));
    }

    case AST_ASSIGNMENT:
    case AST_PLUS_ASSIGNMENT:
    case AST_MINUS_ASSIGNMENT:
    case AST_TIMES_ASSIGNMENT:
    case AST_DIVISION_ASSIGNMENT:
    case AST_MODULO_ASSIGNMENT:
    case AST_INTEGER_DIVISION_ASSIGNMENT:
    case AST_BITWISE_AND_ASSIGNMENT:
    case AST_BITWISE_OR_ASSIGNMENT:
    case AST_BITWISE_XOR_ASSIGNMENT:
    case AST_BITWISE_LEFT_SHIFT_ASSIGNMENT:
    case AST_BITWISE_RIGHT_SHIFT_ASSIGNMENT:
    case AST_BITWISE_UNSIGNED_RIGHT_SHIFT_ASSIGNMENT:
        return assignment(node);

    case AST_PRE_INCREMENT:
    case AST_PRE_DECREMENT:
    case AST_POST_INCREMENT:
    case AST_POST_DECREMENT:
        return increment(node);

    // 'x as T': the type written is the second child, and what it means is
    // on the cast itself
    case AST_CAST:
        return conversion(node, child_of(node, 0), type_at(node));

    default:
        not_yet("the expression '" + text_of(node) + "'");
        return none;
    }
}

// A number is written in decimal, or after '0x', '0b' or '0o', with '_'
// anywhere between its digits; a char between quotes with C's escapes
IrValueId FunctionLowerer::literal(u32 node) {
    u32 type = type_at(node);
    IrTypeId ir = ir_type(type);
    IrValueId none = {0};
    std::string text = text_of(node);
    u64 bits = 0;

    if (failed) {
        return none;
    }

    switch (kind_of(node)) {
    case AST_TRUE:
        bits = 1;
        break;

    case AST_FALSE:
        bits = 0;
        break;

    case AST_CHAR_LITERAL: {
        std::string body = text.size() >= 2 ? text.substr(1, text.size() - 2)
                                            : text;

        if (body.size() >= 2 && body[0] == '\\') {
            switch (body[1]) {
            case 'n': bits = '\n'; break;
            case 't': bits = '\t'; break;
            case 'r': bits = '\r'; break;
            case '0': bits = 0; break;
            default: bits = (u8) body[1]; break;
            }
        } else {
            bits = body.size() > 0 ? (u8) body[0] : 0;
        }

        break;
    }

    default: {
        std::string digits;

        for (char one : text) {
            if (one != '_') {
                digits += one;
            }
        }

        if (is_float_type(type)) {
            double value = std::strtod(digits.c_str(), nullptr);

            if (program->get_types()->get_type(ir)->kind == IR_F32) {
                float narrow = (float) value;
                u32 raw = 0;

                std::memcpy(&raw, &narrow, sizeof(raw));
                bits = raw;
            } else {
                std::memcpy(&bits, &value, sizeof(bits));
            }

            break;
        }

        int base = 10;

        if (digits.size() > 2 && digits[0] == '0') {
            char prefix = digits[1] | 0x20;

            base = prefix == 'x' ? 16 : prefix == 'b' ? 2 : prefix == 'o' ? 8
                                                                         : 10;

            if (base != 10) {
                digits = digits.substr(2);
            }
        }

        bits = std::strtoull(digits.c_str(), nullptr, base);
        break;
    }
    }

    return build.constant(ir, bits, at(node));
}

IrValueId FunctionLowerer::call(u32 node) {
    IrValueId none = {0};
    u32 callee = child_of(node, 0);
    u32 arguments = child_of(node, 1);
    u32 made = type_at(node);

    // 'i32(x)': a builtin has no constructor, so this is a conversion
    if (kind_of(callee) == AST_BUILTIN_TYPE
        || (made != INVALID_TYPE
            && module->get_types()->get_type(made)->kind == TYPE_BUILTIN
            && kind_of(callee) == AST_IDENTIFIER
            && module->get_resolutions()->get(callee)->candidate == 0)) {
        return conversion(node, arguments == 0 ? 0 : child_of(arguments, 0),
                          made);
    }

    if (kind_of(callee) != AST_IDENTIFIER && kind_of(callee) != AST_SCOPE) {
        not_yet("a call through '" + text_of(callee) + "'");
        return none;
    }

    // '::f' and 'alias::f' name the function by their last child
    u32 name = callee;

    if (kind_of(callee) == AST_SCOPE) {
        u32 first = child_of(callee, 0);
        u32 second = first == 0 ? 0
                                : module->get_ast()->get_node(first)->get_sibling();

        name = second == 0 ? first : second;
    }

    Resolution* found = module->get_resolutions()->get(name);
    IrObjectId target = owner->function_of(found->module, found->candidate);

    if (target.index == 0) {
        not_yet("a call to '" + text_of(name) + "', which is not a free "
                "function");
        return none;
    }

    std::vector<IrValueId> values;
    const IrSignature& taken =
        program->get_signature(program->get_function(target).signature);

    for (u32 one = arguments == 0 ? 0 : child_of(arguments, 0); one != 0;
         one = module->get_ast()->get_node(one)->get_sibling()) {
        values.push_back(value_of(one));

        if (failed) {
            return none;
        }
    }

    if (values.size() != taken.parameters.size()) {
        not_yet("a call that leaves a parameter to its default");
        return none;
    }

    return build.call(target, values, at(node));
}

IrValueId FunctionLowerer::conversion(u32 node, u32 operand, u32 to) {
    IrValueId none = {0};
    u32 from = type_at(operand);
    IrValueId value = value_of(operand);
    IrTypeId target = ir_type(to);
    IrTypeId source = ir_type(from);

    if (failed) {
        return none;
    }

    IrTypeTable* types = program->get_types();
    u8 wanted = types->get_type(target)->kind;
    u8 given = types->get_type(source)->kind;

    if (wanted == given) {
        return value;
    }

    bool whole_from = is_whole_type(from)
                   || given == IR_BOOL || given == IR_B8;
    bool whole_to = is_whole_type(to) || wanted == IR_B8;

    if (whole_from && whole_to) {
        IrOpcode opcode = given == IR_BOOL       ? IR_ZEXT
                          : wanted < given       ? IR_TRUNC
                          : is_signed_type(from) ? IR_SEXT
                                                 : IR_ZEXT;

        return build.convert(opcode, target, value, at(node));
    }

    if (whole_from && is_float_type(to)) {
        return build.convert(is_signed_type(from) ? IR_SITOFP : IR_UITOFP,
                             target, value, at(node));
    }

    if (is_float_type(from) && whole_to) {
        return build.convert(is_signed_type(to) ? IR_FPTOSI : IR_FPTOUI,
                             target, value, at(node));
    }

    if (is_float_type(from) && is_float_type(to)) {
        return build.convert(wanted == IR_F64 ? IR_FPEXT : IR_FPTRUNC, target,
                             value, at(node));
    }

    not_yet("the conversion '" + text_of(node) + "'");

    return none;
}

IrValueId FunctionLowerer::arithmetic(u32 node) {
    IrValueId none = {0};
    u32 type = type_at(node);
    IrValueId left = value_of(child_of(node, 0));
    IrValueId right = value_of(child_of(node, 1));

    if (failed) {
        return none;
    }

    return apply(kind_of(node), type, left, right, at(node));
}

IrValueId FunctionLowerer::apply(AstNodeKind kind, u32 type, IrValueId left,
                                 IrValueId right, IrPosition here) {
    IrValueId none = {0};
    IrTypeId ir = ir_type(type);

    // record 0057's floor division: the quotient rounded down, which a
    // truncating one is unless the remainder and the divisor have opposite
    // signs, and then it is one less
    if (kind == AST_INTEGER_DIVISION && is_whole_type(type)
        && is_signed_type(type)) {
        IrTypeId boolean = program->get_types()->scalar(IR_BOOL);
        IrValueId zero = build.constant(ir, 0, here);
        IrValueId one = build.constant(ir, 1, here);
        IrValueId quotient = build.binary(IR_SDIV, ir, left, right, here);
        IrValueId rest = build.binary(IR_SREM, ir, left, right, here);
        IrValueId inexact = build.binary(IR_NE, ir, rest, zero, here);
        IrValueId below = build.binary(IR_SLT, ir, rest, zero, here);
        IrValueId under = build.binary(IR_SLT, ir, right, zero, here);
        IrValueId apart = build.binary(IR_XOR, boolean, below, under, here);
        IrValueId adjust = build.binary(IR_AND, boolean, inexact, apart, here);
        IrValueId lower = build.binary(IR_SUB, ir, quotient, one, here);

        return build.select(ir, adjust, lower, quotient, here);
    }

    if (kind == AST_POWER && is_whole_type(type)) {
        return power(type, left, right, here);
    }

    IrOpcode opcode = opcode_of(kind, type);

    if (failed) {
        return none;
    }

    return build.binary(opcode, ir, left, right, here);
}

// Record 0057's 'a ** b' over a whole number, as the C++ path's helper
// computes it: b multiplications, and for a signed negative exponent the
// quotient 1 / (a ** -b) truncated, which is 0 but for a base of 1 or -1.
// The answer goes through a slot, since it is written inside a region
IrValueId FunctionLowerer::power(u32 type, IrValueId base,
                                 IrValueId exponent, IrPosition here) {
    IrTypeId ir = ir_type(type);
    IrValueId answer = build.entry_slot(ir, here);
    IrValueId turn = build.entry_slot(ir, here);
    IrValueId zero = build.constant(ir, 0, here);
    IrValueId one = build.constant(ir, 1, here);
    bool sign = is_signed_type(type);

    build.store(ir, one, answer, here);
    build.store(ir, zero, turn, here);

    if (sign) {
        IrValueId minus_one = build.binary(IR_SUB, ir, zero, one, here);
        IrValueId negative = build.binary(IR_SLT, ir, exponent, zero, here);

        build.begin_if(negative, here);

        IrValueId two = build.constant(ir, 2, here);
        IrValueId half = build.binary(IR_SREM, ir, exponent, two, here);
        IrValueId even = build.binary(IR_EQ, ir, half, zero, here);
        IrValueId of_minus = build.select(ir, even, one, minus_one, here);
        IrValueId is_minus = build.binary(IR_EQ, ir, base, minus_one, here);
        IrValueId is_one = build.binary(IR_EQ, ir, base, one, here);
        IrValueId small = build.select(ir, is_minus, of_minus, zero, here);

        build.store(ir, build.select(ir, is_one, one, small, here), answer,
                    here);
        build.begin_else();
    }

    IrInstId loop = build.begin_loop(IR_NO_BOUND, here);
    IrValueId done_so_far = build.load(ir, turn, here);
    IrValueId enough = build.binary(sign ? IR_SGE : IR_UGE, ir, done_so_far,
                                    exponent, here);

    build.begin_if(enough, here);
    build.break_to(loop, here);
    build.end();

    IrValueId so_far = build.load(ir, answer, here);

    build.store(ir, build.binary(IR_MUL, ir, so_far, base, here), answer, here);
    build.store(ir, build.binary(IR_ADD, ir, done_so_far, one, here), turn,
                here);
    build.end();

    if (sign) {
        build.end();
    }


    return build.load(ir, answer, here);
}

IrOpcode FunctionLowerer::opcode_of(AstNodeKind kind, u32 type) {
    bool real = is_float_type(type);
    bool sign = is_signed_type(type);

    if (real) {
        switch (kind) {
        case AST_PLUS: return IR_FADD;
        case AST_MINUS: return IR_FSUB;
        case AST_TIMES: return IR_FMUL;
        case AST_DIVISION: return IR_FDIV;
        default:
            not_yet("this operator on floats");
            return IR_OP_NONE;
        }
    }

    if (!is_whole_type(type)
        && !(program->get_types()->get_type(ir_type(type))->kind == IR_BOOL
             && (kind == AST_BITWISE_AND || kind == AST_BITWISE_OR
                 || kind == AST_BITWISE_XOR))) {
        not_yet("arithmetic on this type");
        return IR_OP_NONE;
    }

    switch (kind) {
    case AST_PLUS: return IR_ADD;
    case AST_MINUS: return IR_SUB;
    case AST_TIMES: return IR_MUL;
    case AST_DIVISION: return sign ? IR_SDIV : IR_UDIV;
    case AST_INTEGER_DIVISION: return IR_UDIV;
    case AST_MODULO: return sign ? IR_SREM : IR_UREM;
    case AST_BITWISE_AND: return IR_AND;
    case AST_BITWISE_OR: return IR_OR;
    case AST_BITWISE_XOR: return IR_XOR;
    case AST_BITWISE_LEFT_SHIFT: return IR_SHL;
    case AST_BITWISE_RIGHT_SHIFT: return sign ? IR_ASHR : IR_LSHR;
    case AST_BITWISE_UNSIGNED_RIGHT_SHIFT: return IR_LSHR;
    default:
        not_yet("this operator");
        return IR_OP_NONE;
    }
}

IrValueId FunctionLowerer::comparison(u32 node) {
    IrValueId none = {0};
    u32 type = type_at(child_of(node, 0));
    IrValueId left = value_of(child_of(node, 0));
    IrValueId right = value_of(child_of(node, 1));
    AstNodeKind kind = kind_of(node);
    bool real = is_float_type(type);
    bool sign = is_signed_type(type);
    IrOpcode opcode = IR_OP_NONE;

    if (failed) {
        return none;
    }

    switch (kind) {
    case AST_EQUAL:
        opcode = real ? IR_FEQ : IR_EQ;
        break;

    case AST_NOT_EQUAL:
        opcode = real ? IR_FNE : IR_NE;
        break;

    case AST_LESS_THAN:
        opcode = real ? IR_FLT : sign ? IR_SLT : IR_ULT;
        break;

    case AST_GREATER_THAN:
        opcode = real ? IR_FGT : sign ? IR_SGT : IR_UGT;
        break;

    case AST_LESS_THAN_OR_EQUAL:
        opcode = real ? IR_FLE : sign ? IR_SLE : IR_ULE;
        break;

    default:
        opcode = real ? IR_FGE : sign ? IR_SGE : IR_UGE;
        break;
    }

    return build.binary(opcode, ir_type(type), left, right, at(node));
}

// 'a and b' runs b only when a is true, and 'a or b' only when it is false;
// the answer goes through a slot, since a value never leaves its region
IrValueId FunctionLowerer::logical(u32 node) {
    IrValueId none = {0};
    IrTypeId boolean = program->get_types()->scalar(IR_BOOL);
    IrValueId answer = build.entry_slot(boolean, at(node));
    IrValueId left = value_of(child_of(node, 0));

    if (failed) {
        return none;
    }

    build.store(boolean, left, answer, at(node));
    build.begin_if(kind_of(node) == AST_LOGICAL_AND
                       ? left
                       : build.unary(IR_NOT, boolean, left, at(node)),
                   at(node));

    IrValueId right = value_of(child_of(node, 1));

    if (failed) {
        return none;
    }

    build.store(boolean, right, answer, at(node));
    build.end();

    return build.load(boolean, answer, at(node));
}

IrValueId FunctionLowerer::assignment(u32 node) {
    IrValueId none = {0};
    AstNodeKind kind = kind_of(node);
    u32 type = type_at(child_of(node, 0));
    IrTypeId ir = ir_type(type);
    IrValueId place = place_of(child_of(node, 0));

    if (failed) {
        return none;
    }

    if (kind == AST_ASSIGNMENT) {
        IrValueId value = value_of(child_of(node, 1));

        if (!failed) {
            build.store(ir, value, place, at(node));
        }

        return value;
    }

    AstNodeKind plain = AST_UNKNOWN;

    switch (kind) {
    case AST_PLUS_ASSIGNMENT: plain = AST_PLUS; break;
    case AST_MINUS_ASSIGNMENT: plain = AST_MINUS; break;
    case AST_TIMES_ASSIGNMENT: plain = AST_TIMES; break;
    case AST_DIVISION_ASSIGNMENT: plain = AST_DIVISION; break;
    case AST_MODULO_ASSIGNMENT: plain = AST_MODULO; break;
    case AST_INTEGER_DIVISION_ASSIGNMENT: plain = AST_INTEGER_DIVISION; break;
    case AST_BITWISE_AND_ASSIGNMENT: plain = AST_BITWISE_AND; break;
    case AST_BITWISE_OR_ASSIGNMENT: plain = AST_BITWISE_OR; break;
    case AST_BITWISE_XOR_ASSIGNMENT: plain = AST_BITWISE_XOR; break;
    case AST_BITWISE_LEFT_SHIFT_ASSIGNMENT: plain = AST_BITWISE_LEFT_SHIFT; break;
    case AST_BITWISE_RIGHT_SHIFT_ASSIGNMENT:
        plain = AST_BITWISE_RIGHT_SHIFT;
        break;
    default: plain = AST_BITWISE_UNSIGNED_RIGHT_SHIFT; break;
    }

    IrValueId old = build.load(ir, place, at(node));
    IrValueId right = value_of(child_of(node, 1));

    if (failed) {
        return none;
    }

    IrValueId made = apply(plain, type, old, right, at(node));

    if (!failed) {
        build.store(ir, made, place, at(node));
    }

    return made;
}

IrValueId FunctionLowerer::increment(u32 node) {
    IrValueId none = {0};
    AstNodeKind kind = kind_of(node);
    u32 type = type_at(child_of(node, 0));
    IrTypeId ir = ir_type(type);
    IrValueId place = place_of(child_of(node, 0));

    if (failed) {
        return none;
    }

    if (!is_whole_type(type)) {
        not_yet("'++' or '--' on this type");
        return none;
    }

    bool up = kind == AST_PRE_INCREMENT || kind == AST_POST_INCREMENT;
    IrValueId old = build.load(ir, place, at(node));
    IrValueId one = build.constant(ir, 1, at(node));
    IrValueId made = build.binary(up ? IR_ADD : IR_SUB, ir, old, one, at(node));

    build.store(ir, made, place, at(node));

    return kind == AST_PRE_INCREMENT || kind == AST_PRE_DECREMENT ? made : old;
}

IrValueId FunctionLowerer::place_of(u32 node) {
    IrValueId none = {0};

    if (kind_of(node) == AST_PARENTHESIS) {
        return place_of(child_of(node, 0));
    }

    if (kind_of(node) != AST_IDENTIFIER) {
        not_yet("writing to '" + text_of(node) + "'");
        return none;
    }

    Resolution* found = module->get_resolutions()->get(node);
    std::map<u32, IrValueId>::iterator slot = slots.find(found->candidate);

    if (found->module != module_index || slot == slots.end()) {
        not_yet("the name '" + text_of(node)
                + "', which is not a local of this function");
        return none;
    }

    // record 0035: a 'T&' is the thing it names, and reading one is a step
    // through it -- step 2
    u32 type = module->get_symbols()->get_candidate(found->candidate)->type;
    u8 kind = module->get_types()->get_type(type)->kind;

    if (kind == TYPE_REFERENCE || kind == TYPE_MOVE_REFERENCE) {
        not_yet("a reference");
        return none;
    }

    return slot->second;
}

u32 FunctionLowerer::type_at(u32 node) {
    return module->get_resolutions()->get(node)->type;
}

// the IR type of a value: a scalar, since an aggregate is never a value
// (question 80) and those wait for step 3
IrTypeId FunctionLowerer::ir_type(u32 type) {
    IrTypeId none = {0};

    if (type == INVALID_TYPE) {
        not_yet("an expression the type phase gave no type");
        return none;
    }

    u8 kind = module->get_types()->get_type(type)->kind;

    if (kind == TYPE_REFERENCE || kind == TYPE_MOVE_REFERENCE) {
        not_yet("a reference");
        return none;
    }

    IrTypeId ir = owner->type_of(module_index, type);

    if (ir.index == 0 || program->get_types()->is_aggregate(ir)
        || program->get_types()->get_type(ir)->kind == IR_ARRAY) {
        not_yet("a value of a type that is not a scalar");
        return none;
    }

    return ir;
}

bool FunctionLowerer::is_signed_type(u32 type) {
    Type* entry = module->get_types()->get_type(module->get_types()->value_of(type));

    return entry->kind == TYPE_BUILTIN && is_signed((BuiltinType) entry->subject);
}

bool FunctionLowerer::is_float_type(u32 type) {
    Type* entry = module->get_types()->get_type(module->get_types()->value_of(type));

    return entry->kind == TYPE_BUILTIN
        && (entry->subject == BUILTIN_F32 || entry->subject == BUILTIN_F64);
}

bool FunctionLowerer::is_whole_type(u32 type) {
    Type* entry = module->get_types()->get_type(module->get_types()->value_of(type));

    return entry->kind == TYPE_BUILTIN
        && is_a_whole_number((BuiltinType) entry->subject);
}

// question 86: the token of the node that made the instruction
IrPosition FunctionLowerer::at(u32 node) {
    IrPosition here = {module_index, 0};
    u32 token = node == 0 ? 0 : module->get_ast()->get_node(node)->get_token();

    here.offset = module->get_tokens()->get_token(token).get_offset();

    return here;
}

AstNodeKind FunctionLowerer::kind_of(u32 node) {
    return module->get_ast()->get_node(node)->get_kind();
}

u32 FunctionLowerer::child_of(u32 node, u32 which) {
    u32 child = node == 0 ? 0 : module->get_ast()->get_node(node)->get_children();

    for (u32 i = 0; i < which && child != 0; i++) {
        child = module->get_ast()->get_node(child)->get_sibling();
    }

    return child;
}

std::string FunctionLowerer::text_of(u32 node) {
    return std::string(module->get_token_value(
        module->get_ast()->get_node(node)->get_token()));
}

void FunctionLowerer::not_yet(const std::string& what) {
    if (failed) {
        return;
    }

    failed = true;
    owner->fail("not lowered yet: " + what);
}
