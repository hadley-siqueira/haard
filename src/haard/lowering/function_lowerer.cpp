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
    self.index = 0;
    holder.index = 0;
    destination.index = 0;
    named_result = 0;
    conditional = 0;
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

    u32 own = this->module->get_symbols()->candidate_of(node);
    std::vector<u32> written = signature_of(module, own);
    u8 shape = owner->shape_of(function);
    const IrSignature& taken =
        program->get_signature(program->get_function(function).signature);
    IrTypeId pointer = program->get_types()->pointer(IR_SPACE_GENERIC);
    u32 first = 0;

    returning = written.size() == 0 ? INVALID_TYPE : written.back();

    if (shape & SHAPE_DESTINATION) {
        destination = build.parameter(first++);
        named_result = named_result_of(query.get_block(node));
    }

    if (shape & SHAPE_METHOD) {
        holder = owner->type_of_declaration(module,
                                            owner->holder_of(module, own));
        self = build.entry_slot(pointer, at(node));
        build.store(pointer, build.parameter(first++), self, at(node));
    }

    // every parameter is copied into a slot on entry (question 82); a struct
    // given by value is the caller's copy, and its slot holds its address
    std::vector<u32> parameters = query.get_params(node);

    for (u32 i = 0; i < parameters.size(); i++) {
        u32 candidate = this->module->get_symbols()->candidate_of(parameters[i]);
        IrTypeId type = taken.parameters[first + i];
        IrValueId slot = build.entry_slot(type, at(parameters[i]));

        build.store(type, build.parameter(first + i), slot, at(parameters[i]));
        slots[candidate] = slot;

        if (struct_of(this->module->get_symbols()->get_candidate(candidate)
                          ->type).index != 0
            && !is_reference_type(module, this->module->get_symbols()
                                              ->get_candidate(candidate)
                                              ->type)) {
            indirect.insert(candidate);
        }
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

// The bodies C++ writes for a struct (question 83), each taking the object
// first. What the emitter's C++ does is the rule, since that is what the
// interpreter is checked against
bool FunctionLowerer::lower_structor(const StructorJob& job) {
    IrTypeTable* types = program->get_types();
    IrTypeId pointer = types->pointer(IR_SPACE_GENERIC);
    IrAggregate* aggregate = types->get_aggregate(job.type);
    u32 owning = aggregate->module;
    IrTypeId base = aggregate->base;
    std::vector<IrField> fields = aggregate->fields;
    IrPosition here = {owning, IR_NO_OFFSET};

    begin(owning, job.function);
    here = at(aggregate->declaration);
    holder = job.type;
    self = build.entry_slot(pointer, here);
    build.store(pointer, build.parameter(0), self, here);
    open_scope(false);

    IrValueId object = build.load(pointer, self, here);

    switch (job.kind) {
    // the fields made ready, then the 'init' given the arguments passed on
    case STRUCTOR_CONSTRUCT: {
        prepare(object, job.type);

        if (job.init != 0 && !failed) {
            const IrSignature& taken =
                program->get_signature(program->get_function(job.function)
                                           .signature);
            std::vector<IrValueId> values = {object};

            for (u32 p = 1; p < taken.parameters.size(); p++) {
                values.push_back(build.parameter(p));
            }

            build.call(owner->function_of(owning, job.init), values, here);
        }

        break;
    }

    // C++'s implicit copy: the base and each field by their own copy
    case STRUCTOR_COPY: {
        IrValueId other = build.parameter(1);

        if (base.index != 0) {
            IrValueId to = build.upcast(job.type, base, object, here);
            IrValueId from = build.upcast(job.type, base, other, here);

            if (owner->copies_by_init(base)) {
                copy_construct(to, base, from, here);
            } else {
                copy_fields(base, to, from, here);
            }
        }

        for (u32 i = 0; i < fields.size() && !failed; i++) {
            IrValueId to = build.field(job.type, object, i, here);
            IrValueId from = build.field(job.type, other, i, here);
            IrTypeId one = fields[i].type;

            if (types->is_aggregate(one) || types->get_type(one)->kind == IR_ARRAY) {
                copy_construct(to, one, from, here);
            } else {
                build.store(one, build.load(one, from, here), to, here);
            }
        }

        break;
    }

    // the 'destroy' method, then the fields last to first, then the base
    case STRUCTOR_DESTROY: {
        u32 destroy = owner->destroy_of(job.type);

        if (destroy != 0) {
            build.call(owner->function_of(owning, destroy), {object}, here);
        }

        for (u32 i = fields.size(); i > 0 && !failed; i--) {
            IrTypeId one = fields[i - 1].type;

            if (!owner->has_destructor(one)) {
                continue;
            }

            if (types->get_type(one)->kind == IR_ARRAY) {
                not_yet("an array of structs that end");
                break;
            }

            build.call(owner->structor(STRUCTOR_DESTROY, one, 0),
                       {build.field(job.type, object, i - 1, here)}, here);
        }

        if (base.index != 0 && owner->has_destructor(base)) {
            build.call(owner->structor(STRUCTOR_DESTROY, base, 0),
                       {build.upcast(job.type, base, object, here)}, here);
        }

        break;
    }

    // record 0031: 'a = a' does nothing; otherwise destroy, then the copy
    // 'init' -- or, with no copy 'init', field by field
    case STRUCTOR_ASSIGN:
    case STRUCTOR_MOVE_ASSIGN: {
        IrValueId other = build.parameter(1);
        u32 copy = job.kind == STRUCTOR_MOVE_ASSIGN
                       ? owner->move_init_of(job.type)
                       : owner->copy_init_of(job.type);

        build.begin_if(build.binary(IR_EQ, pointer, object, other, here), here);
        build.ret(here);
        build.end();

        if (copy != 0) {
            u32 destroy = owner->destroy_of(job.type);

            if (destroy != 0) {
                build.call(owner->function_of(owning, destroy), {object}, here);
            }

            build.call(owner->function_of(owning, copy), {object, other}, here);
            break;
        }

        if (base.index != 0) {
            IrValueId to = build.upcast(job.type, base, object, here);
            IrValueId from = build.upcast(job.type, base, other, here);

            if (owner->declares_copy(base)) {
                build.call(owner->structor(STRUCTOR_ASSIGN, base, 0), {to, from},
                           here);
            } else {
                copy_fields(base, to, from, here);
            }
        }

        for (u32 i = 0; i < fields.size(); i++) {
            IrValueId to = build.field(job.type, object, i, here);
            IrValueId from = build.field(job.type, other, i, here);
            IrTypeId one = fields[i].type;

            if (owner->declares_copy(one)) {
                build.call(owner->structor(STRUCTOR_ASSIGN, one, 0), {to, from},
                           here);
            } else if (types->is_aggregate(one)
                       || types->get_type(one)->kind == IR_ARRAY) {
                build.copy(one, to, from, here);
            } else {
                build.store(one, build.load(one, from, here), to, here);
            }
        }

        break;
    }

    default:
        break;
    }

    close_scope(here);

    if (!failed) {
        build.ret(here);
    }

    return !failed;
}

// Record 0026: a field with a value written on it is given it, by every
// constructor; a struct field is built with nothing; the rest hold nothing
void FunctionLowerer::prepare(IrValueId object, IrTypeId type) {
    IrTypeTable* types = program->get_types();
    IrAggregate* aggregate = types->get_aggregate(type);
    IrTypeId base = aggregate->base;
    std::vector<IrField> fields = aggregate->fields;
    AstQuery query;

    query.set_module(module);

    if (base.index != 0) {
        construct_by_default(build.upcast(type, base, object,
                                          at(aggregate->declaration)),
                             base, at(aggregate->declaration));
    }

    for (u32 i = 0; i < fields.size() && !failed; i++) {
        Candidate* declared = module->get_symbols()->get_candidate(
            fields[i].candidate);
        u32 given = query.get_binding_expression(declared->ast_node);
        IrPosition here = at(declared->ast_node);

        if (given == 0) {
            if (types->is_aggregate(fields[i].type)
                || types->get_type(fields[i].type)->kind == IR_ARRAY) {
                construct_by_default(build.field(type, object, i, here),
                                     fields[i].type, here);
            }

            continue;
        }

        open_scope(true);
        initialise(build.field(type, object, i, here), declared->type, given,
                   here);
        close_scope(here);
    }
}

void FunctionLowerer::lower_block(u32 node) {
    if (node == 0) {
        return;
    }

    open_scope(false);

    for (u32 child = child_of(node, 0); child != 0 && !failed;
         child = module->get_ast()->get_node(child)->get_sibling()) {
        // what follows a 'return', a 'break' or a 'continue' in the same
        // block never runs, and the IR has no place to put it
        if (build.ended()) {
            break;
        }

        lower_statement(child);
    }

    close_scope(at(node));
}

void FunctionLowerer::open_scope(bool statement) {
    Scope made;

    made.statement = statement;
    scopes.push_back(made);
}

void FunctionLowerer::close_scope(IrPosition here) {
    if (!failed && !build.ended()) {
        run_cleanups(scopes.size() - 1, here);
    }

    scopes.pop_back();
}

void FunctionLowerer::run_cleanups(u32 depth, IrPosition here) {
    for (u32 s = scopes.size(); s > depth && !failed; s--) {
        std::vector<Cleanup>& cleanups = scopes[s - 1].cleanups;

        for (u32 c = cleanups.size(); c > 0; c--) {
            Cleanup& one = cleanups[c - 1];
            IrTypeId boolean = program->get_types()->scalar(IR_BOOL);

            if (one.flag.index == 0) {
                build.call(one.destructor, {one.place}, here);
                continue;
            }

            build.begin_if(build.load(boolean, one.flag, here), here);
            build.call(one.destructor, {one.place}, here);
            build.store(boolean, build.constant(boolean, 0, here), one.flag,
                        here);
            build.end();
        }
    }
}

void FunctionLowerer::add_cleanup(IrValueId place, IrTypeId type, bool local,
                                  IrPosition here) {
    if (!owner->has_destructor(type)) {
        return;
    }

    Cleanup made;
    u32 at_scope = scopes.size() - 1;

    made.place = place;
    made.destructor = owner->structor(STRUCTOR_DESTROY, type, 0);
    made.flag.index = 0;

    // a local, and a temporary a binding keeps, ends with the block
    while (local && at_scope > 0 && scopes[at_scope].statement) {
        at_scope--;
    }

    if (conditional > 0 && !local) {
        IrTypeId boolean = program->get_types()->scalar(IR_BOOL);

        made.flag = build.entry_flag(here);
        build.store(boolean, build.constant(boolean, 1, here), made.flag,
                    here);
    }

    scopes[at_scope].cleanups.push_back(made);
}

// C++'s named return value, which g++ applies whenever every 'return' gives
// back the same local of the type given back: that local is built in the
// destination and never copied out of it
u32 FunctionLowerer::named_result_of(u32 body) {
    std::vector<u32> found;
    u32 chosen = 0;

    returns_in(body, found);

    for (u32 given : found) {
        while (given != 0 && kind_of(given) == AST_PARENTHESIS) {
            given = child_of(given, 0);
        }

        if (given == 0 || kind_of(given) != AST_IDENTIFIER) {
            return 0;
        }

        Resolution* named = module->get_resolutions()->get(given);
        u32 node = named->candidate == 0
                       ? 0
                       : module->get_symbols()->get_candidate(named->candidate)
                             ->ast_node;

        if (named->module != module_index || node == 0
            || (kind_of(node) != AST_BINDING && kind_of(node) != AST_ASSIGNMENT)
            || (chosen != 0 && chosen != named->candidate)
            || module->get_symbols()->get_candidate(named->candidate)->type
                   != module->get_types()->value_of(returning)) {
            return 0;
        }

        chosen = named->candidate;
    }

    return chosen;
}

void FunctionLowerer::returns_in(u32 node, std::vector<u32>& found) {
    if (node == 0) {
        return;
    }

    if (kind_of(node) == AST_RETURN) {
        found.push_back(child_of(node, 0));
        return;
    }

    // a closure's 'return' is its own
    if (kind_of(node) == AST_CLOSURE) {
        return;
    }

    for (u32 child = child_of(node, 0); child != 0;
         child = module->get_ast()->get_node(child)->get_sibling()) {
        returns_in(child, found);
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
        open_scope(true);
        lower_binding(node);
        close_scope(at(node));
        return;

    case AST_RETURN:
        open_scope(true);
        lower_return(node);
        close_scope(at(node));
        return;

    case AST_BREAK:
        run_cleanups(loops.back().depth, at(node));
        build.break_to(loops.back().broken, at(node));
        return;

    case AST_CONTINUE:
        run_cleanups(loops.back().depth, at(node));

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

    open_scope(true);

    if (kind_of(node) == AST_ASSIGNMENT
        && module->get_symbols()->candidate_of(node) != 0) {
        lower_declaring_assignment(node);
    } else {
        // an expression written for what it does
        value_of(node);
    }

    close_scope(at(node));
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
    // a condition is a statement of its own: its temporaries end before
    // either side runs
    open_scope(true);

    IrValueId test = value_of(condition);

    close_scope(at(node));

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

    open_scope(true);

    IrValueId test = value_of(child_of(node, 0));

    close_scope(at(node));

    if (failed) {
        return;
    }

    build.begin_if(build.unary(IR_NOT, program->get_types()->scalar(IR_BOOL),
                               test, at(node)),
                   at(node));
    build.break_to(loop, at(node));
    build.end();

    Loop around = {loop, loop, false, (u32) scopes.size()};

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

    // what the head declares lives as long as the loop
    open_scope(false);

    for (u32 one = head == 0 ? 0 : child_of(head, 0); one != 0 && !failed;
         one = module->get_ast()->get_node(one)->get_sibling()) {
        lower_statement(one);
    }

    IrInstId loop = build.begin_loop(IR_NO_BOUND, at(node));
    u32 test = condition == 0 ? 0 : child_of(condition, 0);

    if (test != 0) {
        open_scope(true);

        IrValueId value = value_of(test);

        close_scope(at(node));

        if (failed) {
            scopes.pop_back();
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
    Loop around = {loop, body, true, (u32) scopes.size()};

    loops.push_back(around);
    lower_block(block);
    loops.pop_back();
    build.end();

    for (u32 one = increment == 0 ? 0 : child_of(increment, 0);
         one != 0 && !failed;
         one = module->get_ast()->get_node(one)->get_sibling()) {
        open_scope(true);
        value_of(one);
        close_scope(at(node));
    }

    build.end();
    close_scope(at(node));
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

        // the local every 'return' gives back is the destination itself
        IrValueId slot = candidate == named_result
                             ? destination
                             : build.entry_slot(ir, at(binding));
        u32 given = query.get_binding_expression(binding);
        IrTypeId built = is_reference_type(module_index, type)
                             ? IrTypeId{0}
                             : struct_of(type);

        slots[candidate] = slot;

        // record 0026: what is not given a value holds none, but a struct
        // is built with nothing
        if (given != 0) {
            initialise(slot, type, given, at(binding));
        } else if (built.index != 0) {
            construct_by_default(slot, built, at(binding));
        }

        if (built.index != 0 && candidate != named_result) {
            add_cleanup(slot, built, true, at(binding));
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

    IrValueId slot = candidate == named_result ? destination
                                               : build.entry_slot(ir, at(node));
    IrTypeId built = struct_of(type);

    slots[candidate] = slot;
    initialise(slot, type, child_of(node, 1), at(node));

    if (built.index != 0 && candidate != named_result) {
        add_cleanup(slot, built, true, at(node));
    }
}

void FunctionLowerer::lower_return(u32 node) {
    u32 given = child_of(node, 0);

    if (given == 0) {
        run_cleanups(0, at(node));
        build.ret(at(node));
        return;
    }

    // a struct is built where the caller said, unless it is already there
    if (destination.index != 0) {
        u32 named = given;

        while (kind_of(named) == AST_PARENTHESIS) {
            named = child_of(named, 0);
        }

        if (named_result == 0 || kind_of(named) != AST_IDENTIFIER) {
            build_into(destination, struct_of(returning), given);
        }

        if (!failed) {
            run_cleanups(0, at(node));
            build.ret(at(node));
        }

        return;
    }

    // a function giving back a 'T&' gives back a place
    IrValueId value = is_reference_type(module_index, returning)
                          ? reference_to(given)
                          : value_of(given);

    if (!failed) {
        run_cleanups(0, at(node));
        build.ret(result, value, at(node));
    }
}

void FunctionLowerer::initialise(IrValueId place, u32 type, u32 given,
                                 IrPosition here) {
    TypeTable* types = module->get_types();
    u8 kind = types->get_type(type)->kind;

    if (kind == TYPE_REFERENCE || kind == TYPE_MOVE_REFERENCE) {
        IrTypeId built = struct_of(types->value_of(type));
        IrValueId target = {0};

        // record 0068: a temporary a binding refers to lives as long as the
        // binding does
        if (built.index != 0 && !is_place(given)) {
            target = build.entry_slot(built, here);
            build_into(target, built, given);
            add_cleanup(target, built, true, here);
        } else {
            target = reference_to(given);
        }

        if (!failed) {
            build.store(slot_type(type), target, place, here);
        }

        return;
    }

    IrTypeId built = struct_of(type);

    if (built.index != 0) {
        build_into(place, built, given);
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

        if (owner->copies_by_init(slot_type(type))) {
            not_yet("a copy of an array of structs that say how to copy");
        }

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

    // record 0034: an operator a struct declares is a call to its method
    AstNodeKind kind = kind_of(node);

    if (kind != AST_CALL && kind != AST_IDENTIFIER && kind != AST_NEW
        && module->get_resolutions()->get(node)->candidate != 0) {
        if (kind == AST_STRING_LITERAL || kind == AST_ARRAY
            || kind == AST_LIST) {
            not_yet("a literal that builds a class");
            return none;
        }

        IrValueId made = operator_call(node, none);

        if (!failed && returns_reference(node)
            && struct_of(type_at(node)).index == 0) {
            return build.load(ir_type(type_at(node)), made, at(node));
        }

        return made;
    }

    // a struct is never a value: where one is asked for, its address
    if (kind != AST_ADDRESS_OF && struct_of(type_at(node)).index != 0) {
        return reference_to(node);
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
    case AST_DOT:
    case AST_ARROW:
        return read(node);

    case AST_THIS:
        if (self.index == 0) {
            not_yet("'this' outside a method");
            return none;
        }

        return build.load(program->get_types()->pointer(IR_SPACE_GENERIC),
                          self, at(node));

    // '&&x' is x, and only where it is bound does the move mean anything
    case AST_MOVE:
        return is_place(child_of(node, 0)) ? read(node)
                                           : value_of(child_of(node, 0));

    case AST_ADDRESS_OF:
        return place_of(child_of(node, 0));

    case AST_CALL: {
        IrValueId made = call(node, none);

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

IrValueId FunctionLowerer::call(u32 node, IrValueId into) {
    IrValueId none = {0};
    u32 callee = child_of(node, 0);
    u32 arguments = child_of(node, 1);
    u32 first = arguments == 0 ? 0 : child_of(arguments, 0);
    u32 made = type_at(node);

    // 'i32(x)': a builtin has no constructor, so this is a conversion
    if (kind_of(callee) == AST_BUILTIN_TYPE
        || (made != INVALID_TYPE
            && module->get_types()->get_type(made)->kind == TYPE_BUILTIN
            && kind_of(callee) == AST_IDENTIFIER
            && module->get_resolutions()->get(callee)->candidate == 0)) {
        return conversion(node, first, made);
    }

    // Record 0053: 'super(...)' is the base's own 'init', run on the base the
    // constructor has already built
    if (kind_of(callee) == AST_SUPER) {
        Resolution* chosen = module->get_resolutions()->get(node);
        IrObjectId target = owner->function_of(chosen->module,
                                               chosen->candidate);
        IrTypeId base = holder.index == 0
                            ? IrTypeId{0}
                            : program->get_types()->get_aggregate(holder)->base;
        std::vector<IrValueId> values;

        if (target.index == 0 || base.index == 0) {
            not_yet("a 'super' whose base is not a struct");
            return none;
        }

        values.push_back(upcast_to(
            build.load(program->get_types()->pointer(IR_SPACE_GENERIC), self,
                       at(node)),
            holder, base, at(node)));

        if (!arguments_for(chosen->module, chosen->candidate, first, values)) {
            return none;
        }

        return build.call(target, values, at(node));
    }

    // '::f' and 'alias::f' name the function by their last child, and 'p.f'
    // by its right
    u32 name = callee;

    if (kind_of(callee) == AST_SCOPE) {
        u32 left = child_of(callee, 0);
        u32 second = left == 0 ? 0
                               : module->get_ast()->get_node(left)->get_sibling();

        name = second == 0 ? left : second;
    } else if (kind_of(callee) == AST_DOT || kind_of(callee) == AST_ARROW) {
        name = child_of(callee, 1);
    }

    if (kind_of(name) == AST_GENERIC_NAME) {
        name = child_of(name, 0);
    }

    // Record 0045: a call whose callee names a type is a construction, of
    // the 'init' the typer chose and wrote on the call
    if ((kind_of(name) == AST_IDENTIFIER || kind_of(name) == AST_SCOPE)
        && module->get_resolutions()->get(name)->candidate == 0
        && kind_of(callee) != AST_DOT && kind_of(callee) != AST_ARROW
        && made != INVALID_TYPE
        && module->get_types()->get_type(made)->kind == TYPE_NAMED) {
        IrTypeId built = struct_of(made);
        Resolution* chosen = module->get_resolutions()->get(node);
        std::vector<IrValueId> values;

        if (built.index == 0) {
            not_yet("a construction of a class, a union or an enum");
            return none;
        }

        IrValueId place = into;

        if (place.index == 0) {
            place = build.entry_slot(built, at(node));
        }

        if (chosen->candidate == 0) {
            construct_by_default(place, built, at(node));
        } else if (arguments_for(chosen->module, chosen->candidate, first,
                                 values)) {
            construct(place, built, chosen->candidate, values, at(node));
        }

        if (into.index == 0) {
            add_cleanup(place, built, false, at(node));
        }

        return place;
    }

    if (kind_of(name) != AST_IDENTIFIER) {
        not_yet("a call through '" + text_of(callee) + "'");
        return none;
    }

    Resolution* found = module->get_resolutions()->get(name);
    IrObjectId target = owner->function_of(found->module, found->candidate);

    if (target.index == 0) {
        u32 holding = found->candidate == 0
                          ? 0
                          : owner->holder_of(found->module, found->candidate);

        not_yet(holding != 0 ? "a call of a class's method"
                             : "a call to '" + text_of(name)
                                   + "', which is not a function");
        return none;
    }

    u8 shape = owner->shape_of(target);
    std::vector<IrValueId> values;
    IrValueId place = into;
    IrTypeId built = {0};

    // question 80: a struct given back is built where the caller says, a
    // temporary when the caller says nowhere
    if (shape & SHAPE_DESTINATION) {
        built = struct_of(signature_of(found->module, found->candidate).back(),
                          found->module);

        if (place.index == 0) {
            place = build.entry_slot(built, at(node));
        }

        values.push_back(place);
    }

    if (shape & SHAPE_METHOD) {
        values.push_back(receiver_of(callee, found->module, found->candidate));
    }

    if (failed
        || !arguments_for(found->module, found->candidate, first, values)) {
        return none;
    }

    IrValueId given = build.call(target, values, at(node));

    if (shape & SHAPE_DESTINATION) {
        if (into.index == 0) {
            add_cleanup(place, built, false, at(node));
        }

        return place;
    }

    return given;
}

bool FunctionLowerer::arguments_for(u32 callee_module, u32 candidate,
                                    u32 first,
                                    std::vector<IrValueId>& values) {
    std::vector<u32> written = signature_of(callee_module, candidate);
    u32 at_parameter = 0;

    for (u32 one = first; one != 0;
         one = module->get_ast()->get_node(one)->get_sibling(), at_parameter++) {
        u32 wanted = at_parameter + 1 < written.size() ? written[at_parameter]
                                                       : INVALID_TYPE;
        IrTypeId built = wanted == INVALID_TYPE
                             ? IrTypeId{0}
                             : struct_of(wanted, callee_module);

        // a 'T&' is given a place; a struct by value a copy the caller makes
        // and ends with the statement (question 83, Itanium's rule)
        if (wanted != INVALID_TYPE && is_reference_type(callee_module, wanted)) {
            values.push_back(reference_to(one));
        } else if (built.index != 0) {
            IrValueId copy = build.entry_slot(built, at(one));

            build_into(copy, built, one);
            add_cleanup(copy, built, false, at(one));
            values.push_back(copy);
        } else {
            values.push_back(value_of(one));
        }

        if (failed) {
            return false;
        }
    }

    if (at_parameter + 1 < written.size()) {
        not_yet("a call that leaves a parameter to its default");
        return false;
    }

    return true;
}

// a method's object: 'this' inside one, the left of a '.' or a '->' --
// through it when it is a pointer -- or 'super', the object as its base
IrValueId FunctionLowerer::receiver_of(u32 callee, u32 callee_module,
                                       u32 candidate) {
    IrValueId none = {0};
    u32 declaration = owner->holder_of(callee_module, candidate);
    IrTypeId target = owner->type_of_declaration(callee_module, declaration);
    IrTypeId pointer = program->get_types()->pointer(IR_SPACE_GENERIC);
    IrValueId object = none;
    IrTypeId from = holder;

    if (compilation->get_module(callee_module)->get_ast()
            ->get_node(declaration)->get_kind() != AST_STRUCT) {
        not_yet("a call of a class's method");
        return none;
    }

    u32 left = kind_of(callee) == AST_DOT || kind_of(callee) == AST_ARROW
                   ? child_of(callee, 0)
                   : 0;

    if (left == 0 || kind_of(left) == AST_SUPER || kind_of(left) == AST_THIS) {
        object = build.load(pointer, self, at(callee));
    } else if (kind_of_type(type_at(left)) == TYPE_POINTER) {
        object = value_of(left);
        from = element_of(type_at(left));
    } else {
        object = reference_to(left);
        from = owner->type_of(module_index,
                              module->get_types()->value_of(type_at(left)));
    }

    if (failed) {
        return none;
    }

    return upcast_to(object, from, target, at(callee));
}

IrValueId FunctionLowerer::upcast_to(IrValueId pointer, IrTypeId from,
                                     IrTypeId to, IrPosition here) {
    return from.index == to.index ? pointer
                                  : build.upcast(from, to, pointer, here);
}

// Record 0034: 'a + b' is 'a.operator+(b)', the emitter's reading too: the
// left operand is the object, through it when it is a pointer
IrValueId FunctionLowerer::operator_call(u32 node, IrValueId into) {
    IrValueId none = {0};
    Resolution* chosen = module->get_resolutions()->get(node);
    IrObjectId target = owner->function_of(chosen->module, chosen->candidate);

    if (target.index == 0) {
        not_yet("an operator a class overloads, or a construction");
        return none;
    }

    u32 left = child_of(node, 0);
    u32 declaration = owner->holder_of(chosen->module, chosen->candidate);
    IrTypeId target_type = owner->type_of_declaration(chosen->module,
                                                      declaration);
    u8 shape = owner->shape_of(target);
    std::vector<IrValueId> values;
    IrValueId place = into;
    IrTypeId built = {0};

    if (shape & SHAPE_DESTINATION) {
        built = struct_of(signature_of(chosen->module, chosen->candidate).back(),
                          chosen->module);

        if (place.index == 0) {
            place = build.entry_slot(built, at(node));
        }

        values.push_back(place);
    }

    IrValueId object = kind_of_type(type_at(left)) == TYPE_POINTER
                           ? value_of(left)
                           : reference_to(left);
    IrTypeId from = kind_of_type(type_at(left)) == TYPE_POINTER
                        ? element_of(type_at(left))
                        : struct_of(type_at(left));

    if (failed) {
        return none;
    }

    values.push_back(upcast_to(object, from, target_type, at(node)));

    if (!arguments_for(chosen->module, chosen->candidate,
                       module->get_ast()->get_node(left)->get_sibling(),
                       values)) {
        return none;
    }

    IrValueId given = build.call(target, values, at(node));

    if (shape & SHAPE_DESTINATION) {
        if (into.index == 0) {
            add_cleanup(place, built, false, at(node));
        }

        return place;
    }

    return given;
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

    // what the right side makes it makes on this path only
    conditional++;

    IrValueId right = value_of(child_of(node, 1));

    conditional--;

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
    IrTypeId built = struct_of(type);

    // record 0031: a struct that says how to copy itself is assigned by
    // destroying and copying; '&&x' by destroying and moving (Hadley,
    // 2026-09-30); one that says nothing a field at a time, as C++'s own
    // '=' does, which for a struct of plain fields is its bytes
    if (built.index != 0 && kind == AST_ASSIGNMENT) {
        u32 right = child_of(node, 1);
        IrValueId to = place_of(child_of(node, 0));
        bool moved = kind_of(right) == AST_MOVE
                  && owner->move_init_of(built) != 0;
        IrValueId from = reference_to(right);
        IrTypeId given = struct_of(type_at(right));

        if (failed) {
            return none;
        }

        from = upcast_to(from, given.index == 0 ? built : given, built,
                         at(node));

        if (moved) {
            build.call(owner->structor(STRUCTOR_MOVE_ASSIGN, built, 0),
                       {to, from}, at(node));
        } else if (owner->declares_copy(built)) {
            build.call(owner->structor(STRUCTOR_ASSIGN, built, 0), {to, from},
                       at(node));
        } else {
            build.copy(built, to, from, at(node));
        }

        return none;
    }

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

// 'new T' is C++'s 'new T()', which gives a scalar its zero and a struct
// with no 'init' zero before it is built; 'new T[n]' gives n of them
// unwritten. 'new T(...)' runs the 'init' the typer chose
IrValueId FunctionLowerer::allocation(u32 node) {
    IrValueId none = {0};
    IrTypeId element = element_of(type_at(node));
    u32 written = child_of(node, 0);
    IrTypeTable* types = program->get_types();

    if (failed) {
        return none;
    }

    if (types->is_aggregate(element) && !owner->lowerable(element)) {
        not_yet("a 'new' of a class, a union, an enum or a tuple");
        return none;
    }

    IrValueId bytes = build.size(element, usize(), at(node));

    if (kind_of(written) == AST_ARRAY_TYPE) {
        u32 length = child_of(written, 1);
        IrValueId count = value_of(length);

        if (types->is_aggregate(element) && owner->constructs(element)) {
            not_yet("a 'new' of an array of structs built with nothing");
            return none;
        }

        if (failed) {
            return none;
        }

        bytes = build.binary(IR_MUL, usize(), bytes,
                             to_width(count, type_at(length), usize(), at(node)),
                             at(node));

        return build.call(owner->allocator(), {bytes}, at(node));
    }

    IrValueId made = build.call(owner->allocator(), {bytes}, at(node));

    if (types->is_aggregate(element)) {
        Resolution* chosen = module->get_resolutions()->get(node);
        u32 arguments = child_of(node, 1);
        std::vector<IrValueId> values;

        if (chosen->candidate != 0) {
            if (arguments_for(chosen->module, chosen->candidate,
                              arguments == 0 ? 0 : child_of(arguments, 0),
                              values)) {
                construct(made, element, chosen->candidate, values, at(node));
            }

            return made;
        }

        if (!owner->has_init(element)) {
            zero(made, element, at(node));
        }

        construct_by_default(made, element, at(node));

        return made;
    }

    if (child_of(node, 1) != 0 && child_of(child_of(node, 1), 0) != 0) {
        not_yet("a 'new' of a scalar given arguments");
        return none;
    }

    IrValueId zero_value = types->get_type(element)->kind == IR_PTR
                               ? build.null(at(node))
                               : build.constant(element, 0, at(node));

    build.store(element, zero_value, made, at(node));

    return made;
}

// a struct is ended first, unless the pointer is null, where C++'s 'delete'
// does nothing at all
IrValueId FunctionLowerer::release(u32 node) {
    IrValueId none = {0};
    u32 operand = child_of(node, 0);
    IrTypeId element = element_of(type_at(operand));
    IrValueId pointer = value_of(operand);

    if (failed) {
        return none;
    }

    if (program->get_types()->is_aggregate(element)
        && !owner->lowerable(element)) {
        not_yet("a 'delete' of a class, a union, an enum or a tuple");
        return none;
    }

    if (owner->has_destructor(element)) {
        if (kind_of(node) == AST_DELETE_ARRAY) {
            not_yet("a 'delete[]' of structs that end");
            return none;
        }

        build.begin_if(build.binary(IR_NE,
                                    program->get_types()->pointer(
                                        IR_SPACE_GENERIC),
                                    pointer, build.null(at(node)), at(node)),
                       at(node));
        build.call(owner->structor(STRUCTOR_DESTROY, element, 0), {pointer},
                   at(node));
        build.end();
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

// What 'node' means, built in 'place'. A call builds there itself; a place
// is copied, or moved from when '&&' says so and the struct has a move
// 'init'; a struct derived from this one is copied as this one, C++'s slice
void FunctionLowerer::build_into(IrValueId place, IrTypeId type, u32 node) {
    switch (kind_of(node)) {
    case AST_PARENTHESIS:
        build_into(place, type, child_of(node, 0));
        return;

    case AST_CALL:
        if (!returns_reference(node)) {
            call(node, place);
            return;
        }

        break;

    case AST_MOVE: {
        u32 moved = owner->move_init_of(type);

        if (moved != 0 && is_place(child_of(node, 0))) {
            IrValueId from = place_of(child_of(node, 0));

            if (!failed) {
                construct(place, type, moved, {from}, at(node));
            }

            return;
        }

        break;
    }

    default:
        if (module->get_resolutions()->get(node)->candidate != 0
            && !is_place(node)) {
            operator_call(node, place);
            return;
        }

        break;
    }

    if (!is_place(node)) {
        not_yet("a struct built from '" + text_of(node) + "'");
        return;
    }

    IrValueId from = place_of(node);
    IrTypeId given = struct_of(type_at(node));

    if (failed) {
        return;
    }

    copy_construct(place,
                   type,
                   upcast_to(from, given.index == 0 ? type : given, type,
                             at(node)),
                   at(node));
}

IrValueId FunctionLowerer::materialise(u32 node) {
    IrTypeId type = struct_of(type_at(node));
    IrValueId made = build.entry_slot(type, at(node));

    build_into(made, type, node);
    add_cleanup(made, type, false, at(node));

    return made;
}

void FunctionLowerer::construct(IrValueId place, IrTypeId type, u32 init,
                                const std::vector<IrValueId>& arguments,
                                IrPosition here) {
    if (init == 0 && !owner->constructs(type)) {
        return;
    }

    std::vector<IrValueId> values = {place};

    values.insert(values.end(), arguments.begin(), arguments.end());
    build.call(owner->structor(STRUCTOR_CONSTRUCT, type, init), values, here);
}

// C++'s default constructor: the 'init' that takes nothing; none at all when
// the struct writes no 'init', and an empty one when every 'init' it writes
// takes something (record 0053) -- the fields made ready either way
void FunctionLowerer::construct_by_default(IrValueId place, IrTypeId type,
                                           IrPosition here) {
    IrTypeTable* types = program->get_types();

    if (types->get_type(type)->kind == IR_ARRAY) {
        if (owner->constructs(type)) {
            not_yet("an array of structs built with nothing");
        }

        return;
    }

    if (!owner->constructs(type)) {
        return;
    }

    u32 empty = owner->empty_init_of(type);
    IrAggregate* aggregate = types->get_aggregate(type);
    Module* owning = compilation->get_module(aggregate->module);
    AstQuery query;

    query.set_module(owning);

    // an 'init' answering to nothing because each parameter has a default
    for (u32 member : query.get_members(aggregate->declaration)) {
        if (empty != 0
            || owning->get_ast()->get_node(member)->get_kind() != AST_FUNCTION
            || query.get_declaration_name(member) != "init"
            || query.get_params(member).size() == 0) {
            continue;
        }

        bool every = true;

        for (u32 param : query.get_params(member)) {
            every = every && query.get_binding_expression(param) != 0;
        }

        if (every) {
            not_yet("an 'init' that answers to nothing by its defaults");
            return;
        }
    }

    construct(place, type, empty, {}, here);
}

void FunctionLowerer::copy_construct(IrValueId place, IrTypeId type,
                                     IrValueId from, IrPosition here) {
    if (!owner->copies_by_init(type)) {
        build.copy(type, place, from, here);
        return;
    }

    if (program->get_types()->get_type(type)->kind == IR_ARRAY) {
        not_yet("a copy of an array of structs that say how to copy");
        return;
    }

    u32 copy = owner->copy_init_of(type);

    build.call(copy != 0 ? owner->structor(STRUCTOR_CONSTRUCT, type, copy)
                         : owner->structor(STRUCTOR_COPY, type, 0),
               {place, from}, here);
}

void FunctionLowerer::copy_fields(IrTypeId type, IrValueId to, IrValueId from,
                                  IrPosition here) {
    IrTypeTable* types = program->get_types();
    IrAggregate* aggregate = types->get_aggregate(type);
    IrTypeId base = aggregate->base;
    std::vector<IrField> fields = aggregate->fields;

    if (base.index != 0) {
        copy_fields(base, build.upcast(type, base, to, here),
                    build.upcast(type, base, from, here), here);
    }

    for (u32 i = 0; i < fields.size(); i++) {
        IrValueId into = build.field(type, to, i, here);
        IrValueId out_of = build.field(type, from, i, here);
        IrTypeId one = fields[i].type;

        if (types->is_aggregate(one) || types->get_type(one)->kind == IR_ARRAY) {
            build.copy(one, into, out_of, here);
        } else {
            build.store(one, build.load(one, out_of, here), into, here);
        }
    }
}

void FunctionLowerer::zero(IrValueId place, IrTypeId type, IrPosition here) {
    IrTypeTable* types = program->get_types();
    const IrType* entry = types->get_type(type);

    if (entry->kind == IR_ARRAY) {
        IrTypeId element = entry->element;
        u32 length = entry->subject;
        IrValueId turn = build.entry_slot(usize(), here);
        IrValueId one = build.constant(usize(), 1, here);

        build.store(usize(), build.constant(usize(), 0, here), turn, here);

        IrInstId loop = build.begin_loop(length, here);
        IrValueId i = build.load(usize(), turn, here);

        build.begin_if(build.binary(IR_UGE, usize(), i,
                                    build.constant(usize(), length, here),
                                    here),
                       here);
        build.break_to(loop, here);
        build.end();
        zero(build.index(element, place, i, here), element, here);
        build.store(usize(), build.binary(IR_ADD, usize(), i, one, here), turn,
                    here);
        build.end();
        return;
    }

    if (types->is_aggregate(type)) {
        IrAggregate* aggregate = types->get_aggregate(type);
        IrTypeId base = aggregate->base;
        std::vector<IrField> fields = aggregate->fields;

        if (base.index != 0) {
            zero(build.upcast(type, base, place, here), base, here);
        }

        for (u32 i = 0; i < fields.size(); i++) {
            zero(build.field(type, place, i, here), fields[i].type, here);
        }

        return;
    }

    build.store(type,
                entry->kind == IR_PTR ? build.null(here)
                                      : build.constant(type, 0, here),
                place, here);
}

IrValueId FunctionLowerer::read(u32 node) {
    IrValueId none = {0};
    u32 type = type_at(node);
    IrValueId place = place_of(node);

    if (failed) {
        return none;
    }

    // an array decays, and a struct is never a value: both are their place
    if (kind_of_type(type) == TYPE_ARRAY || struct_of(type).index != 0) {
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
            if (returns_reference(node)) {
                return operator_call(node, none);
            }

            break;
        }

        return place_of_index(node);

    case AST_DOT:
    case AST_ARROW:
        return place_of_member(node);

    case AST_CALL:
        if (returns_reference(node)) {
            return call(node, none);
        }

        break;

    default:
        break;
    }

    not_yet("writing to '" + text_of(node) + "'");

    return none;
}

// 'p.x' and 'p->x', a '.' through a pointer as well: the field, found up
// through the bases, of the struct the left is or points at
IrValueId FunctionLowerer::place_of_member(u32 node) {
    IrValueId none = {0};
    u32 left = child_of(node, 0);
    u32 right = child_of(node, 1);
    Resolution* named = module->get_resolutions()->get(right);
    IrTypeId pointer = program->get_types()->pointer(IR_SPACE_GENERIC);

    if (named->candidate == 0
        || (SymbolKind) compilation->get_module(named->module)->get_symbols()
                   ->get_candidate(named->candidate)->kind
               != SYMBOL_FIELD) {
        not_yet("the member '" + text_of(right) + "', which is not a field");
        return none;
    }

    IrValueId object = none;
    IrTypeId type = {0};

    if (kind_of(left) == AST_SUPER || kind_of(left) == AST_THIS) {
        object = build.load(pointer, self, at(node));
        type = holder;
    } else if (kind_of_type(type_at(left)) == TYPE_POINTER) {
        object = value_of(left);
        type = element_of(type_at(left));
    } else {
        object = reference_to(left);
        type = type_at(left) == INVALID_TYPE
                   ? IrTypeId{0}
                   : owner->type_of(module_index,
                                    module->get_types()->value_of(type_at(left)));
    }

    if (failed) {
        return none;
    }

    if (type.index == 0 || !program->get_types()->is_aggregate(type)) {
        not_yet("a member of '" + text_of(left) + "'");
        return none;
    }

    return field_of(object, type, named->module, named->candidate, at(node));
}

IrValueId FunctionLowerer::field_of(IrValueId pointer, IrTypeId type,
                                    u32 field_module, u32 candidate,
                                    IrPosition here) {
    IrValueId none = {0};
    IrTypeTable* types = program->get_types();

    for (IrTypeId at_type = type; at_type.index != 0;) {
        IrAggregate* aggregate = types->get_aggregate(at_type);

        for (u32 i = 0; i < aggregate->fields.size(); i++) {
            if (aggregate->module == field_module
                && aggregate->fields[i].candidate == candidate) {
                return build.field(at_type, pointer, i, here);
            }
        }

        IrTypeId base = aggregate->base;

        if (base.index == 0) {
            break;
        }

        pointer = build.upcast(at_type, base, pointer, here);
        at_type = base;
    }

    not_yet("a field the struct does not hold");

    return none;
}

// a local's slot or a global's object, and through it what a 'T&' names; a
// field of the object a method is running on
IrValueId FunctionLowerer::place_of_name(u32 node) {
    IrValueId none = {0};
    Resolution* found = module->get_resolutions()->get(node);
    Module* owning = compilation->get_module(found->module);
    IrTypeId pointer = program->get_types()->pointer(IR_SPACE_GENERIC);

    if (found->candidate != 0
        && (SymbolKind) owning->get_symbols()->get_candidate(found->candidate)
                   ->kind
               == SYMBOL_FIELD) {
        if (self.index == 0) {
            not_yet("a field outside a method");
            return none;
        }

        return field_of(build.load(pointer, self, at(node)), holder,
                        found->module, found->candidate, at(node));
    }

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

    if (is_reference_type(found->module,
                          owning->get_symbols()->get_candidate(found->candidate)
                              ->type)
        || (found->module == module_index
            && indirect.count(found->candidate) > 0)) {
        return build.load(pointer, place, at(node));
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
    case AST_DOT:
    case AST_ARROW:
        return true;

    case AST_INDEX:
        return module->get_resolutions()->get(node)->candidate == 0
            || returns_reference(node);

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

    if (struct_of(type_at(node)).index != 0) {
        return materialise(node);
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

// a call's function, or the method an operator a struct declares is
bool FunctionLowerer::callee_of(u32 node, u32& callee_module, u32& candidate) {
    u32 name = node;

    if (kind_of(node) == AST_CALL) {
        u32 callee = child_of(node, 0);

        name = callee;

        if (kind_of(callee) == AST_SCOPE) {
            u32 first = child_of(callee, 0);
            u32 second = first == 0
                             ? 0
                             : module->get_ast()->get_node(first)->get_sibling();

            name = second == 0 ? first : second;
        } else if (kind_of(callee) == AST_DOT || kind_of(callee) == AST_ARROW) {
            name = child_of(callee, 1);
        } else if (kind_of(callee) != AST_IDENTIFIER) {
            return false;
        }

        if (kind_of(name) == AST_GENERIC_NAME) {
            name = child_of(name, 0);
        }
    }

    Resolution* found = module->get_resolutions()->get(name);

    if (found->candidate == 0
        || owner->function_of(found->module, found->candidate).index == 0) {
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

    if (ir.index == 0 || !owner->lowerable(ir)) {
        not_yet("a value of a class, a union, an enum or a tuple");
        return none;
    }

    return ir;
}

IrTypeId FunctionLowerer::struct_of(u32 type) {
    return struct_of(type, module_index);
}

IrTypeId FunctionLowerer::struct_of(u32 type, u32 owning) {
    IrTypeId none = {0};

    if (type == INVALID_TYPE) {
        return none;
    }

    TypeTable* types = compilation->get_module(owning)->get_types();
    u32 value = types->value_of(type);

    if (types->get_type(value)->kind != TYPE_NAMED) {
        return none;
    }

    IrTypeId ir = owner->type_of(owning, value);

    return ir.index != 0 && program->get_types()->get_type(ir)->kind == IR_STRUCT
                   && owner->lowerable(ir)
               ? ir
               : none;
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
