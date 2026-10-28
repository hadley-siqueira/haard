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
    returning = INVALID_TYPE;
    failed = false;
}

void FunctionLowerer::begin(u32 module, IrObjectId function) {
    module_index = module;
    this->module = compilation->get_module(module);
    build.set_program(program);
    build.begin_function(function);
    result = program->get_signature(program->get_function(function).signature)
                 .result;
}

bool FunctionLowerer::lower(u32 module, u32 node, IrObjectId function) {
    AstQuery query;

    begin(module, function);
    query.set_module(this->module);

    std::vector<u32> written = signature_of(
        module, this->module->get_symbols()->candidate_of(node));

    returning = written.size() == 0 ? INVALID_TYPE : written.back();

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

bool FunctionLowerer::lower_globals(u32 module, IrObjectId function) {
    AstQuery query;

    begin(module, function);
    query.set_module(this->module);

    for (u32 statement : query.get_global_variables()) {
        for (u32 binding = child_of(statement, 0); binding != 0 && !failed;
             binding = this->module->get_ast()->get_node(binding)->get_sibling()) {
            u32 candidate = this->module->get_symbols()->candidate_of(binding);
            u32 given = query.get_binding_expression(binding);

            if (given != 0) {
                initialise(build.address(owner->global_of(module, candidate),
                                         at(binding)),
                           this->module->get_symbols()->get_candidate(candidate)
                               ->type,
                           given, at(binding));
            }
        }
    }

    if (!failed) {
        build.ret(at(0));
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
        IrTypeId ir = slot_type(type);

        if (failed) {
            return;
        }

        IrValueId slot = build.entry_slot(ir, at(binding));
        u32 given = query.get_binding_expression(binding);

        slots[candidate] = slot;

        // record 0026: what is not given a value holds none
        if (given != 0) {
            initialise(slot, type, given, at(binding));
        }
    }
}

void FunctionLowerer::lower_declaring_assignment(u32 node) {
    u32 candidate = module->get_symbols()->candidate_of(node);
    u32 type = module->get_symbols()->get_candidate(candidate)->type;
    IrTypeId ir = slot_type(type);

    if (failed) {
        return;
    }

    IrValueId slot = build.entry_slot(ir, at(node));

    slots[candidate] = slot;
    initialise(slot, type, child_of(node, 1), at(node));
}

void FunctionLowerer::lower_return(u32 node) {
    u32 given = child_of(node, 0);

    if (given == 0) {
        build.ret(at(node));
        return;
    }

    // a function giving back a 'T&' gives back a place
    IrValueId value = is_reference_type(module_index, returning)
                          ? reference_to(given)
                          : value_of(given);

    if (!failed) {
        build.ret(result, value, at(node));
    }
}

void FunctionLowerer::initialise(IrValueId place, u32 type, u32 given,
                                 IrPosition here) {
    TypeTable* types = module->get_types();
    u8 kind = types->get_type(type)->kind;

    if (kind == TYPE_REFERENCE || kind == TYPE_MOVE_REFERENCE) {
        IrValueId target = reference_to(given);

        if (!failed) {
            build.store(slot_type(type), target, place, here);
        }

        return;
    }

    if (kind == TYPE_ARRAY && kind_of(given) == AST_ARRAY) {
        u32 element = types->get_argument(types->get_type(type)->first_argument);
        IrTypeId each = element_of(type);
        u64 at_index = 0;

        for (u32 one = child_of(given, 0); one != 0 && !failed;
             one = module->get_ast()->get_node(one)->get_sibling()) {
            IrValueId into = build.index(
                each, place, build.constant(usize(), at_index++, here), here);

            initialise(into, element, one, here);
        }

        return;
    }

    if (kind == TYPE_ARRAY) {
        IrValueId from = place_of(given);

        if (!failed) {
            build.copy(slot_type(type), place, from, here);
        }

        return;
    }

    IrValueId value = value_of(given);

    if (!failed) {
        build.store(ir_type(type), value, place, here);
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

    case AST_NULL_LITERAL:
        return build.null(at(node));

    case AST_STRING_LITERAL:
        return string_literal(node);

    case AST_ARRAY:
        return brace_literal(node);

    case AST_PARENTHESIS:
        return value_of(child_of(node, 0));

    case AST_IDENTIFIER:
    case AST_DEREFERENCE:
    case AST_INDEX:
        return read(node);

    // '&&x' is x, and only where it is bound does the move mean anything
    case AST_MOVE:
        return is_place(child_of(node, 0)) ? read(node)
                                           : value_of(child_of(node, 0));

    case AST_ADDRESS_OF:
        return place_of(child_of(node, 0));

    case AST_CALL: {
        IrValueId made = call(node);

        if (!failed && returns_reference(node)) {
            return build.load(ir_type(type_at(node)), made, at(node));
        }

        return made;
    }

    case AST_SIZEOF:
        return size_of(node);

    case AST_NEW:
        return allocation(node);

    case AST_DELETE:
    case AST_DELETE_ARRAY:
        return release(node);

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
        std::string body = unescape(
            text.size() >= 2 ? text.substr(1, text.size() - 2) : text);

        bits = body.size() > 0 ? (u8) body[0] : 0;
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
    std::vector<u32> written = signature_of(found->module, found->candidate);

    // a 'T&' parameter is given a place
    for (u32 one = arguments == 0 ? 0 : child_of(arguments, 0); one != 0;
         one = module->get_ast()->get_node(one)->get_sibling()) {
        u32 at_parameter = (u32) values.size();

        values.push_back(at_parameter + 1 < written.size()
                                 && is_reference_type(found->module,
                                                      written[at_parameter])
                             ? reference_to(one)
                             : value_of(one));

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
    AstNodeKind kind = kind_of(node);
    u32 left_type = type_at(child_of(node, 0));
    u32 right_type = type_at(child_of(node, 1));
    IrValueId left = value_of(child_of(node, 0));
    IrValueId right = value_of(child_of(node, 1));

    if (failed) {
        return none;
    }

    // record 0076: a pointer moves by a whole number, and two are
    // subtracted into the elements between them
    bool left_pointer = kind_of_type(left_type) == TYPE_POINTER;
    bool right_pointer = kind_of_type(right_type) == TYPE_POINTER;

    if ((kind == AST_PLUS || kind == AST_MINUS)
        && (left_pointer || right_pointer)) {
        if (left_pointer && right_pointer) {
            return distance(left_type, left, right, at(node));
        }

        return left_pointer ? step(left_type, left, right, right_type,
                                   kind == AST_MINUS, at(node))
                            : step(right_type, right, left, left_type, false,
                                   at(node));
    }

    return apply(kind, type, left, right, at(node));
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

    // 'null' says nothing of the pointer it is compared with
    u32 type = type_at(kind_of(child_of(node, 0)) == AST_NULL_LITERAL
                           ? child_of(node, 1)
                           : child_of(node, 0));
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

    IrValueId made =
        kind_of_type(type) == TYPE_POINTER
                && (plain == AST_PLUS || plain == AST_MINUS)
            ? step(type, old, right, type_at(child_of(node, 1)),
                   plain == AST_MINUS, at(node))
            : apply(plain, type, old, right, at(node));

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

    bool pointer = kind_of_type(type) == TYPE_POINTER;

    if (!is_whole_type(type) && !pointer) {
        not_yet("'++' or '--' on this type");
        return none;
    }

    bool up = kind == AST_PRE_INCREMENT || kind == AST_POST_INCREMENT;
    IrValueId old = build.load(ir, place, at(node));
    IrValueId made = {0};

    if (pointer) {
        made = build.index(element_of(type), old,
                           build.constant(usize(), up ? 1 : (u64) -1, at(node)),
                           at(node));
    } else {
        IrValueId one = build.constant(ir, 1, at(node));

        made = build.binary(up ? IR_ADD : IR_SUB, ir, old, one, at(node));
    }

    build.store(ir, made, place, at(node));

    return kind == AST_PRE_INCREMENT || kind == AST_PRE_DECREMENT ? made : old;
}

IrValueId FunctionLowerer::string_literal(u32 node) {
    std::string text = text_of(node);

    return build.address(
        owner->text(unescape(text.size() >= 2 ? text.substr(1, text.size() - 2)
                                              : text)),
        at(node));
}

IrValueId FunctionLowerer::size_of(u32 node) {
    IrValueId none = {0};
    IrTypeId measured = owner->type_of(module_index, type_at(child_of(node, 0)));
    IrTypeId given = ir_type(type_at(node));

    if (failed) {
        return none;
    }

    if (measured.index == 0) {
        not_yet("the size of a type with no layout");
        return none;
    }

    return build.size(measured, given, at(node));
}

// 'new T' is C++'s 'new T()', which gives a scalar its zero; 'new T[n]'
// gives n of them unwritten. A class and its 'init' wait for step 3
IrValueId FunctionLowerer::allocation(u32 node) {
    IrValueId none = {0};
    IrTypeId element = element_of(type_at(node));
    u32 written = child_of(node, 0);

    if (failed) {
        return none;
    }

    if (program->get_types()->is_aggregate(element)) {
        not_yet("a 'new' of a struct, a class or a union");
        return none;
    }

    IrValueId bytes = build.size(element, usize(), at(node));

    if (kind_of(written) == AST_ARRAY_TYPE) {
        u32 length = child_of(written, 1);
        IrValueId count = value_of(length);

        if (failed) {
            return none;
        }

        bytes = build.binary(IR_MUL, usize(), bytes,
                             to_width(count, type_at(length), usize(), at(node)),
                             at(node));

        return build.call(owner->allocator(), {bytes}, at(node));
    }

    if (child_of(node, 1) != 0 && child_of(child_of(node, 1), 0) != 0) {
        not_yet("a 'new' given arguments");
        return none;
    }

    IrValueId made = build.call(owner->allocator(), {bytes}, at(node));
    IrValueId zero = program->get_types()->get_type(element)->kind == IR_PTR
                         ? build.null(at(node))
                         : build.constant(element, 0, at(node));

    build.store(element, zero, made, at(node));

    return made;
}

IrValueId FunctionLowerer::release(u32 node) {
    IrValueId none = {0};
    u32 operand = child_of(node, 0);
    IrTypeId element = element_of(type_at(operand));
    IrValueId pointer = value_of(operand);

    if (failed) {
        return none;
    }

    if (program->get_types()->is_aggregate(element)) {
        not_yet("a 'delete' of a struct, a class or a union");
        return none;
    }

    build.call(owner->deallocator(), {pointer}, at(node));

    return none;
}

IrValueId FunctionLowerer::brace_literal(u32 node) {
    IrValueId none = {0};
    u32 type = type_at(node);
    IrTypeId ir = slot_type(type);

    if (failed) {
        return none;
    }

    IrValueId made = build.entry_slot(ir, at(node));

    initialise(made, type, node, at(node));

    return made;
}

IrValueId FunctionLowerer::read(u32 node) {
    IrValueId none = {0};
    u32 type = type_at(node);
    IrValueId place = place_of(node);

    if (failed) {
        return none;
    }

    if (kind_of_type(type) == TYPE_ARRAY) {
        return place;
    }

    IrTypeId ir = ir_type(type);

    return failed ? none : build.load(ir, place, at(node));
}

IrValueId FunctionLowerer::step(u32 pointer, IrValueId base, IrValueId count,
                                u32 count_type, bool back, IrPosition here) {
    IrValueId none = {0};
    IrTypeId element = element_of(pointer);
    IrValueId by = to_width(count, count_type, usize(), here);

    if (failed) {
        return none;
    }

    if (back) {
        by = build.unary(IR_NEG, usize(), by, here);
    }

    return build.index(element, base, by, here);
}

IrValueId FunctionLowerer::distance(u32 pointer, IrValueId left,
                                    IrValueId right, IrPosition here) {
    IrValueId none = {0};
    IrTypeId element = element_of(pointer);

    if (failed) {
        return none;
    }

    IrValueId from = build.convert(IR_PTRTOINT, usize(), left, here);
    IrValueId to = build.convert(IR_PTRTOINT, usize(), right, here);
    IrValueId bytes = build.binary(IR_SUB, usize(), from, to, here);

    return build.binary(IR_SDIV, usize(), bytes,
                        build.size(element, usize(), here), here);
}

IrValueId FunctionLowerer::place_of(u32 node) {
    IrValueId none = {0};

    switch (kind_of(node)) {
    case AST_PARENTHESIS:
    case AST_MOVE:
        return place_of(child_of(node, 0));

    case AST_IDENTIFIER:
        return place_of_name(node);

    case AST_DEREFERENCE:
        return value_of(child_of(node, 0));

    case AST_INDEX:
        if (module->get_resolutions()->get(node)->candidate != 0) {
            not_yet("an operator a class overloads, or a construction");
            return none;
        }

        return place_of_index(node);

    case AST_CALL:
        if (returns_reference(node)) {
            return call(node);
        }

        break;

    default:
        break;
    }

    not_yet("writing to '" + text_of(node) + "'");

    return none;
}

// a local's slot or a global's object, and through it what a 'T&' names
IrValueId FunctionLowerer::place_of_name(u32 node) {
    IrValueId none = {0};
    Resolution* found = module->get_resolutions()->get(node);
    std::map<u32, IrValueId>::iterator slot = slots.find(found->candidate);
    IrObjectId global = owner->global_of(found->module, found->candidate);
    IrValueId place = none;

    if (found->module == module_index && slot != slots.end()) {
        place = slot->second;
    } else if (global.index != 0) {
        place = build.address(global, at(node));
    } else {
        not_yet("the name '" + text_of(node)
                + "', which is not a local or a global");
        return none;
    }

    Module* owning = compilation->get_module(found->module);

    if (is_reference_type(found->module,
                          owning->get_symbols()->get_candidate(found->candidate)
                              ->type)) {
        return build.load(program->get_types()->pointer(IR_SPACE_GENERIC),
                          place, at(node));
    }

    return place;
}

// A pointer is indexed as it is; a fixed array is checked first, question
// 9b, the index read as unsigned so a negative one is out of range too
IrValueId FunctionLowerer::place_of_index(u32 node) {
    IrValueId none = {0};
    u32 base = child_of(node, 0);
    u32 which = child_of(node, 1);
    u32 type = type_at(base);
    u8 kind = kind_of_type(type);

    if (kind != TYPE_POINTER && kind != TYPE_ARRAY) {
        not_yet("indexing a '" + text_of(base) + "' that is not a pointer or "
                "a fixed array");
        return none;
    }

    IrTypeId element = element_of(type);
    IrValueId address = kind == TYPE_POINTER ? value_of(base)
                                             : reference_to(base);
    IrValueId offset = value_of(which);

    if (failed) {
        return none;
    }

    offset = to_width(offset, type_at(which), usize(), at(node));

    if (kind == TYPE_ARRAY) {
        TypeTable* types = module->get_types();
        u32 length = types->get_type(types->value_of(type))->subject;

        build.check_index(usize(), offset,
                          build.constant(usize(), length, at(node)), at(node));
    }

    return build.index(element, address, offset, at(node));
}

bool FunctionLowerer::is_place(u32 node) {
    switch (kind_of(node)) {
    case AST_PARENTHESIS:
    case AST_MOVE:
        return is_place(child_of(node, 0));

    case AST_IDENTIFIER:
    case AST_DEREFERENCE:
    case AST_INDEX:
        return true;

    case AST_CALL:
        return returns_reference(node);

    default:
        return false;
    }
}

IrValueId FunctionLowerer::reference_to(u32 node) {
    IrValueId none = {0};

    if (is_place(node)) {
        return place_of(node);
    }

    if (kind_of(node) == AST_ARRAY) {
        return brace_literal(node);
    }

    IrTypeId type = ir_type(type_at(node));
    IrValueId value = value_of(node);

    if (failed) {
        return none;
    }

    IrValueId made = build.entry_slot(type, at(node));

    build.store(type, value, made, at(node));

    return made;
}

bool FunctionLowerer::callee_of(u32 node, u32& callee_module, u32& candidate) {
    if (kind_of(node) != AST_CALL) {
        return false;
    }

    u32 callee = child_of(node, 0);
    u32 name = callee;

    if (kind_of(callee) == AST_SCOPE) {
        u32 first = child_of(callee, 0);
        u32 second = first == 0 ? 0
                                : module->get_ast()->get_node(first)->get_sibling();

        name = second == 0 ? first : second;
    } else if (kind_of(callee) != AST_IDENTIFIER) {
        return false;
    }

    Resolution* found = module->get_resolutions()->get(name);

    if (owner->function_of(found->module, found->candidate).index == 0) {
        return false;
    }

    callee_module = found->module;
    candidate = found->candidate;

    return true;
}

std::vector<u32> FunctionLowerer::signature_of(u32 callee_module,
                                               u32 candidate) {
    Module* owning = compilation->get_module(callee_module);

    if (candidate == 0) {
        return std::vector<u32>();
    }

    return owning->get_types()->get_arguments(
        owning->get_symbols()->get_candidate(candidate)->type);
}

bool FunctionLowerer::returns_reference(u32 node) {
    u32 callee_module = 0;
    u32 candidate = 0;

    if (!callee_of(node, callee_module, candidate)) {
        return false;
    }

    std::vector<u32> written = signature_of(callee_module, candidate);

    return written.size() > 0
        && is_reference_type(callee_module, written.back());
}

u32 FunctionLowerer::type_at(u32 node) {
    return module->get_resolutions()->get(node)->type;
}

IrTypeId FunctionLowerer::ir_type(u32 type) {
    IrTypeId none = {0};

    if (type == INVALID_TYPE) {
        not_yet("an expression the type phase gave no type");
        return none;
    }

    IrTypeId ir = owner->type_of(module_index,
                                 module->get_types()->value_of(type));

    if (ir.index == 0 || program->get_types()->is_aggregate(ir)
        || program->get_types()->get_type(ir)->kind == IR_ARRAY) {
        not_yet("a value of a type that is not a scalar");
        return none;
    }

    return ir;
}

IrTypeId FunctionLowerer::slot_type(u32 type) {
    IrTypeId none = {0};

    if (type == INVALID_TYPE) {
        not_yet("a binding the type phase gave no type");
        return none;
    }

    IrTypeId ir = owner->type_of(module_index, type);

    if (ir.index == 0 || program->get_types()->is_aggregate(ir)) {
        not_yet("a value of a type that is not a scalar");
        return none;
    }

    return ir;
}

IrTypeId FunctionLowerer::element_of(u32 type) {
    IrTypeId none = {0};
    TypeTable* types = module->get_types();
    Type* entry = types->get_type(types->value_of(type));
    IrTypeId ir = owner->type_of(module_index,
                                 types->get_argument(entry->first_argument));

    if (ir.index == 0) {
        not_yet("a pointer to something with no layout");
        return none;
    }

    return ir;
}

IrTypeId FunctionLowerer::usize() {
    return program->get_types()->integer(owner->get_target()->pointer_size);
}

IrValueId FunctionLowerer::to_width(IrValueId value, u32 type, IrTypeId to,
                                    IrPosition here) {
    IrTypeTable* types = program->get_types();
    IrTypeId from = ir_type(type);

    if (failed || from.index == to.index) {
        return value;
    }

    u8 given = types->get_type(from)->kind;
    u8 wanted = types->get_type(to)->kind;

    if (given > wanted) {
        return build.convert(IR_TRUNC, to, value, here);
    }

    return build.convert(is_signed_type(type) ? IR_SEXT : IR_ZEXT, to, value,
                         here);
}

u8 FunctionLowerer::kind_of_type(u32 type) {
    TypeTable* types = module->get_types();

    return type == INVALID_TYPE ? TYPE_NONE
                                : types->get_type(types->value_of(type))->kind;
}

bool FunctionLowerer::is_reference_type(u32 owning, u32 type) {
    if (type == INVALID_TYPE) {
        return false;
    }

    u8 kind = compilation->get_module(owning)->get_types()->get_type(type)->kind;

    return kind == TYPE_REFERENCE || kind == TYPE_MOVE_REFERENCE;
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

// C's escapes, the octal and hex ones aside, and Haard's '\$' (Hadley,
// 2026-09-30)
std::string FunctionLowerer::unescape(const std::string& body) {
    std::string made;

    for (size_t i = 0; i < body.size(); i++) {
        if (body[i] != '\\' || i + 1 == body.size()) {
            made += body[i];
            continue;
        }

        switch (body[++i]) {
        case 'n': made += '\n'; break;
        case 't': made += '\t'; break;
        case 'r': made += '\r'; break;
        case 'a': made += '\a'; break;
        case 'b': made += '\b'; break;
        case 'f': made += '\f'; break;
        case 'v': made += '\v'; break;
        case '0': made += '\0'; break;
        default: made += body[i]; break;
        }
    }

    return made;
}

void FunctionLowerer::not_yet(const std::string& what) {
    if (failed) {
        return;
    }

    failed = true;
    owner->fail("not lowered yet: " + what);
}
