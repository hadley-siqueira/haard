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

    // a method filling a slot is given its object as the class that
    // introduced the slot, and sees it as its own
    if (shape & SHAPE_METHOD) {
        IrTypeId given = owner->receiver_type(module, own);
        IrValueId object = build.parameter(first++);

        holder = owner->type_of_declaration(module,
                                            owner->holder_of(module, own));
        self = build.entry_slot(pointer, at(node));

        if (given.index != holder.index) {
            object = build.downcast(given, holder, object, at(node));
        }

        build.store(pointer, object, self, at(node));
    }

    if (shape & SHAPE_DESTINATION) {
        destination = build.parameter(first++);
        named_result = named_result_of(query.get_block(node));
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

        u32 written = this->module->get_symbols()->get_candidate(candidate)
                          ->type;

        // and a fixed array decays to the address of its first element, as
        // C++ passes one (record 0077, step 2): the caller's, not a copy
        if ((struct_of(written).index != 0 || kind_of_type(written) == TYPE_ARRAY)
            && !is_reference_type(module, written)) {
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

// A global is built as a local is, in the object that starts zeroed: what its
// value leaves behind ends with it, and what runs when it ends goes on the
// list, in the order they are built.
//
// Record 0068: a temporary a reference is given lives as long as the reference
// does, which here is past this function, so it has an object of its own
bool FunctionLowerer::lower_globals(u32 module, IrObjectId function,
                                    std::vector<GlobalEnd>& ending) {
    AstQuery query;
    TypeTable* types = compilation->get_module(module)->get_types();

    begin(module, function);
    query.set_module(this->module);

    for (u32 statement : query.get_global_variables()) {
        for (u32 binding = child_of(statement, 0); binding != 0 && !failed;
             binding = this->module->get_ast()->get_node(binding)->get_sibling()) {
            u32 candidate = this->module->get_symbols()->candidate_of(binding);
            u32 type = this->module->get_symbols()->get_candidate(candidate)->type;
            u32 given = query.get_binding_expression(binding);
            IrObjectId global = owner->global_of(module, candidate);
            IrPosition here = at(binding);
            bool reference = is_reference_type(module, type);
            u32 value = types->value_of(type);
            IrTypeId held = owner->type_of(module, value);

            if (reference && (!is_place(given) || converts(given, struct_of(value)))) {
                IrObjectId temporary = owner->global_temporary(held);

                open_scope(false);
                initialise(build.address(temporary, here), value, given, here);
                close_scope(here);
                build.store(owner->type_of(module, type),
                            build.address(temporary, here),
                            build.address(global, here), here);
                global = temporary;
            } else if (given != 0) {
                open_scope(false);
                initialise(build.address(global, here), type, given, here);
                close_scope(here);

                if (reference) {
                    continue;
                }
            } else {
                construct_by_default(build.address(global, here), held, here);
            }

            if (owner->has_destructor(held)) {
                ending.push_back({global, held});
            }
        }
    }

    if (!failed) {
        build.ret(at(0));
    }

    return !failed;
}

bool FunctionLowerer::lower_global_ends(u32 module, IrObjectId function,
                                        const std::vector<GlobalEnd>& ending) {
    begin(module, function);

    for (u32 i = (u32) ending.size(); i > 0 && !failed; i--) {
        destroy(build.address(ending[i - 1].global, at(0)), ending[i - 1].type,
                at(0));
    }

    if (!failed) {
        build.ret(at(0));
    }

    return !failed;
}

// Record 0051, what the C++ path's shim is: the list built with nothing, an
// element built from each 'char*' and given to its 'add', and 'main' given the
// list. The class is asked for 'add' by name, so nothing here knows 'Array' or
// 'String'; and the shim being the one call no Haard source wrote, what such a
// call would have been told is asked here
bool FunctionLowerer::lower_entry(u32 main, IrObjectId function) {
    begin(0, function);

    IrTypeTable* types = program->get_types();
    IrTypeId pointer = types->pointer(IR_SPACE_GENERIC);
    IrTypeId count_type = types->scalar(IR_B32);
    IrPosition here = at(module->get_symbols()->get_candidate(main)->ast_node);
    std::vector<u32> written = signature_of(0, main);
    IrTypeId list = struct_of(written[0]);

    if (list.index == 0) {
        owner->fail("the entry module declares no 'main' that takes nothing, "
                    "an (i32, char**), or one list of arguments");
        return false;
    }

    std::string list_name = types->get_aggregate(list)->name;
    u32 adder = owner->method_named(list, "add", 1, 0);
    u32 holder_module = types->get_aggregate(list)->module;

    if (adder == 0) {
        owner->fail(list_name + " declares no 'add', so 'main' cannot be given "
                    "its arguments as one of them");
        return false;
    }

    IrTypeId element = struct_of(signature_of(holder_module, adder)[0],
                                 holder_module);

    if (element.index == 0) {
        owner->fail("'add' does not take a class, so 'main' has nothing to "
                    "build an argument out of");
        return false;
    }

    bool by_value = !is_reference_type(0, written[0]);

    if (by_value && owner->destroy_of(list) != 0
        && owner->copy_init_of(list) == 0) {
        owner->fail(list_name + " owns something and says nothing about being "
                    "copied, so 'main' cannot take it by value");
        return false;
    }

    TypeTable* written_types = module->get_types();
    u32 init = owner->init_from(
        element, 0, written_types->pointer(written_types->builtin(BUILTIN_CHAR)));

    if (init == 0) {
        not_yet("a 'main' whose list holds a class with no 'init' of a 'char*'");
        return false;
    }

    open_scope(false);

    IrValueId args = build.entry_slot(list, here);

    construct_by_default(args, list, here);
    add_cleanup(args, list, true, here);

    IrValueId turn = build.entry_slot(count_type, here);
    IrValueId one = build.constant(count_type, 1, here);
    IrValueId held = build.entry_slot(element, here);

    build.store(count_type, build.constant(count_type, 0, here), turn, here);

    IrInstId loop = build.begin_loop(IR_NO_BOUND, here);
    IrValueId at_index = build.load(count_type, turn, here);

    build.begin_if(build.binary(IR_SGE, count_type, at_index, build.parameter(0),
                                here),
                   here);
    build.break_to(loop, here);
    build.end();

    IrValueId text = build.load(
        pointer,
        build.index(pointer, build.parameter(1),
                    build.convert(IR_SEXT, usize(), at_index, here), here),
        here);

    construct(held, element, init, {text}, here);

    if (owner->shape_of(function_of(holder_module, adder)) & SHAPE_DESTINATION) {
        not_yet("a 'main' whose list's 'add' gives back a struct");
        return false;
    }

    call_method(holder_module, adder, {args, held}, false, here);

    if (owner->has_destructor(element)) {
        destroy(held, element, here);
    }

    build.store(count_type, build.binary(IR_ADD, count_type, at_index, one, here),
                turn, here);
    build.end();

    // by value it is a copy this call makes, ended before the list is
    IrValueId given = args;

    if (by_value) {
        given = build.entry_slot(list, here);
        copy_construct(given, list, args, here);
        add_cleanup(given, list, false, here);
    }

    IrValueId answer = build.call(owner->function_of(0, main), {given}, here);

    close_scope(here);

    if (failed) {
        return false;
    }

    if (result.index == 0) {
        build.ret(here);
    } else {
        build.ret(result, answer, here);
    }

    return true;
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

    // a class's destructor fills slot 0, so it is given its object as the
    // class at the top of the chain
    IrValueId given = build.parameter(0);
    bool is_class = types->get_type(job.type)->kind == IR_CLASS;

    if (job.kind == STRUCTOR_DESTROY && is_class
        && owner->top_class(job.type).index != job.type.index) {
        given = build.downcast(owner->top_class(job.type), job.type, given,
                               here);
    }

    build.store(pointer, given, self, here);
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

            build.call(function_of(owning, job.init), values, here);
        }

        break;
    }

    // C++'s implicit copy: the base and each field by their own copy
    case STRUCTOR_COPY: {
        IrValueId other = build.parameter(1);

        if (owner->is_tagged(job.type)) {
            copy_alive(job.type, object, other, here);
            break;
        }

        if (base.index != 0) {
            IrValueId to = build.upcast(job.type, base, object, here);
            IrValueId from = build.upcast(job.type, base, other, here);

            if (owner->copies_by_init(base)) {
                copy_construct(to, base, from, here);
            } else {
                copy_fields(base, to, from, here);
            }
        }

        if (is_class) {
            store_vptr(object, job.type, here);
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
        u32 destroy_method = owner->destroy_of(job.type);

        if (is_class) {
            store_vptr(object, job.type, here);
        }

        if (destroy_method != 0) {
            call_method(owning, destroy_method, {object}, false, here);
        }

        if (owner->is_tagged(job.type)) {
            destroy_alive(job.type, object, here);
            break;
        }

        for (u32 i = fields.size(); i > 0 && !failed; i--) {
            IrTypeId one = fields[i - 1].type;

            if (!owner->has_destructor(one)) {
                continue;
            }

            destroy(build.field(job.type, object, i - 1, here), one, here);
        }

        if (base.index != 0 && owner->has_destructor(base)) {
            destroy(build.upcast(job.type, base, object, here), base, here);
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

        // an enum: the member alive ended, then the other's copied in
        if (owner->is_tagged(job.type)) {
            destroy_alive(job.type, object, here);
            copy_alive(job.type, object, other, here);
            break;
        }

        if (copy != 0) {
            u32 destroy_method = owner->destroy_of(job.type);

            if (destroy_method != 0) {
                call_method(owning, destroy_method, {object}, false, here);
            }

            build.call(function_of(owning, copy), {object, other}, here);
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
            } else {
                assign_value(one, to, from, here);
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

    if (types->get_type(type)->kind == IR_CLASS) {
        store_vptr(object, type, at(aggregate->declaration));
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
                destroy(one.place, one.type, here);
                continue;
            }

            build.begin_if(build.load(boolean, one.flag, here), here);
            destroy(one.place, one.type, here);
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
    made.type = type;
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
        lower_switch(node);
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

// Record 0043's switch, over an integer, a char or an enum -- the others the
// sugar pass made a chain of 'if's. A 'block' with an 'if' per group of
// cases, each leaving the block when it is done: no fall through, and a
// 'break' inside still leaves the loop around it. The subject is read once,
// and an enum that carries something is switched over by its tag; what a
// case binds refers into the payload of its variant
void FunctionLowerer::lower_switch(u32 node) {
    u32 subject = child_of(node, 0);
    u32 type = module->get_types()->value_of(type_at(subject));
    IrTypeId tagged = struct_of(type);
    IrTypeId boolean = program->get_types()->scalar(IR_BOOL);
    IrValueId whole = {0};
    IrValueId tag = {0};
    IrTypeId tag_type = {0};

    // the subject's temporaries live as long as the switch does
    open_scope(false);

    if (tagged.index != 0) {
        whole = reference_to(subject);
        tag_type = program->get_types()->get_aggregate(tagged)->fields[0].type;

        if (!failed) {
            tag = build.load(tag_type, build.field(tagged, whole, 0, at(node)),
                             at(node));
        }
    } else {
        tag_type = ir_type(type);
        tag = value_of(subject);
    }

    if (failed) {
        return;
    }

    IrInstId region = build.begin_block(at(node));
    std::vector<u32> group;
    u32 fallback = 0;
    u32 last = 0;

    for (u32 child = module->get_ast()->get_node(subject)->get_sibling();
         child != 0; child = module->get_ast()->get_node(child)->get_sibling()) {
        if (kind_of(child) == AST_DEFAULT) {
            fallback = child_of(child, 0);
        } else {
            last = child;
        }
    }

    // Record 0043: a switch over an enum names every variant or has a
    // 'default', so with none its last group is what is left and runs
    // without a test -- and the end of the switch is not reached when every
    // case gives back, which the IR has to see
    bool exhaustive = fallback == 0
                   && module->get_types()->get_type(type)->kind == TYPE_NAMED;

    for (u32 child = module->get_ast()->get_node(subject)->get_sibling();
         child != 0 && !failed;
         child = module->get_ast()->get_node(child)->get_sibling()) {
        if (kind_of(child) == AST_DEFAULT) {
            continue;
        }

        u32 pattern = child_of(child, 0);
        u32 block = module->get_ast()->get_node(pattern)->get_sibling();

        group.push_back(child);

        if (block == 0) {
            continue;
        }

        if (exhaustive && child == last) {
            open_scope(false);
            bind_captures(type, group, tagged, whole, tag, tag_type);
            lower_block(block);
            close_scope(at(child));
            group.clear();
            break;
        }

        IrValueId any = {0};

        for (u32 one : group) {
            IrValueId label = case_label(type, child_of(one, 0), tag_type);

            if (failed) {
                return;
            }

            IrValueId same = build.binary(IR_EQ, tag_type, tag, label,
                                          at(one));

            any = any.index == 0 ? same
                                 : build.binary(IR_OR, boolean, any, same,
                                                at(one));
        }

        build.begin_if(any, at(child));
        open_scope(false);
        bind_captures(type, group, tagged, whole, tag, tag_type);
        lower_block(block);
        close_scope(at(child));

        if (!build.ended()) {
            build.break_to(region, at(child));
        }

        build.end();
        group.clear();
    }

    if (fallback != 0 && !failed) {
        lower_block(fallback);
    }

    build.end();
    close_scope(at(node));
}

// what a case is compared with: a variant's tag, or the value written
IrValueId FunctionLowerer::case_label(u32 type, u32 pattern,
                                      IrTypeId tag_type) {
    u32 variant_module = 0;
    u32 variant = variant_of_pattern(type, pattern, variant_module);

    if (variant != 0) {
        return build.constant(tag_type, tag_of_variant(variant_module, variant),
                              at(pattern));
    }

    return value_of(pattern);
}

// The variant a case names, found by its name among the variants of the
// enum switched over, as the emitter finds it: 'Click(x, y)', 'E.Click' and
// a bare 'Click' (record 0072); 0 over a switch that is not an enum's
u32 FunctionLowerer::variant_of_pattern(u32 type, u32 pattern,
                                        u32& in_module) {
    Type* entry = module->get_types()->get_type(type);

    if (entry->kind != TYPE_NAMED) {
        return 0;
    }

    Module* holder = compilation->get_module(entry->module);
    u32 declaration =
        holder->get_symbols()->get_candidate(entry->subject)->ast_node;
    u32 name = pattern;

    if (kind_of(name) == AST_CALL) {
        name = child_of(name, 0);
    }

    if (kind_of(name) == AST_DOT) {
        name = child_of(name, 1);
    }

    std::string wanted = text_of(name);
    AstQuery theirs;

    theirs.set_module(holder);

    for (u32 member : theirs.get_variants(declaration)) {
        if (theirs.get_declaration_name(member) == wanted) {
            in_module = entry->module;
            return holder->get_symbols()->candidate_of(member);
        }
    }

    return 0;
}

// Each name a group of cases binds, in the scope the cases share: a
// reference to the field of the payload member of the variant that matched.
// Every case in a group binds the same names for the same types (Hadley's
// rule), so which member it comes out of is a question about the tag
void FunctionLowerer::bind_captures(u32 type, const std::vector<u32>& group,
                                    IrTypeId tagged, IrValueId whole,
                                    IrValueId tag, IrTypeId tag_type) {
    AstQuery query;

    query.set_module(module);

    if (group.size() == 0 || tagged.index == 0) {
        return;
    }

    std::vector<u32> captures = query.get_captures(group.back());

    if (captures.size() == 0) {
        return;
    }

    SymbolTable* table = module->get_symbols();
    IrTypeTable* types = program->get_types();
    IrAggregate* aggregate = types->get_aggregate(tagged);
    IrTypeId payload = aggregate->fields[1].type;
    IrTypeId pointer = types->pointer(IR_SPACE_GENERIC);
    u32 scope = table->scope_owned_by(group.back());

    for (u32 i = 0; i < captures.size() && !failed; i++) {
        u32 candidate = capture_named(scope, captures[i]);

        if (candidate == 0) {
            not_yet("a case that binds a name nothing declared");
            return;
        }

        IrPosition here = at(captures[i]);
        IrValueId address = {0};

        for (u32 one : group) {
            u32 variant_module = 0;
            u32 variant = variant_of_pattern(type, child_of(one, 0),
                                             variant_module);

            IrAggregate* members = types->get_aggregate(payload);
            u32 member = 0;

            while (member < members->fields.size()
                   && members->fields[member].candidate != variant) {
                member++;
            }

            if (member == members->fields.size()) {
                not_yet("a case that binds from a variant carrying nothing");
                return;
            }

            IrTypeId carried = members->fields[member].type;
            IrValueId inside = build.field(
                carried,
                build.field(payload, build.field(tagged, whole, 1, here),
                            member, here),
                i, here);

            if (address.index == 0) {
                address = inside;
            } else {
                IrValueId is = build.binary(
                    IR_EQ, tag_type, tag,
                    build.constant(tag_type,
                                   tag_of_variant(variant_module, variant),
                                   here),
                    here);

                address = build.select(pointer, is, inside, address, here);
            }
        }

        u32 type = table->get_candidate(candidate)->type;
        IrValueId slot = build.entry_slot(pointer, here);

        // record 0043: a capture is a reference to what the variant holds
        if (!is_reference_type(module_index, type)) {
            IrTypeId held = slot_type(type);

            if (failed) {
                return;
            }

            IrValueId copy = build.entry_slot(held, here);

            if (struct_of(type).index != 0) {
                copy_construct(copy, held, address, here);
                add_cleanup(copy, held, true, here);
            } else {
                build.store(held, build.load(held, address, here), copy, here);
            }

            slots[candidate] = copy;
            continue;
        }

        build.store(pointer, address, slot, here);
        slots[candidate] = slot;
    }
}

// the candidate a capture's name was declared as, by name in its case's scope
u32 FunctionLowerer::capture_named(u32 scope, u32 name) {
    SymbolTable* table = module->get_symbols();
    std::string wanted = text_of(name);

    if (scope == 0) {
        return 0;
    }

    for (u32 symbol = table->get_scope(scope)->symbols; symbol != 0;
         symbol = table->get_symbol(symbol)->sibling_or_next) {
        if (std::string(module->get_strings()->get_text(
                table->get_symbol(symbol)->name))
            == wanted) {
            return table->get_symbol(symbol)->candidates;
        }
    }

    return 0;
}

void FunctionLowerer::lower_while(u32 node) {
    IrInstId loop = build.begin_loop(IR_NO_BOUND, at(node));
    u32 condition = child_of(node, 0);

    while (kind_of(condition) == AST_PARENTHESIS) {
        condition = child_of(condition, 0);
    }

    // 'while true' has no test: only a 'break' or a 'return' leaves it, so
    // when it has none what follows it is reached by nothing -- 'unwrap'
    // (record 0070) ends in one, and its function has no end to fall off
    if (kind_of(condition) != AST_TRUE) {
        open_scope(true);

        IrValueId test = value_of(child_of(node, 0));

        close_scope(at(node));

        if (failed) {
            return;
        }

        build.begin_if(build.unary(IR_NOT,
                                   program->get_types()->scalar(IR_BOOL), test,
                                   at(node)),
                       at(node));
        build.break_to(loop, at(node));
        build.end();
    }

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
        // a struct, or a fixed array of them, is built and ended
        IrTypeId built = is_reference_type(module_index, type)
                             ? IrTypeId{0}
                             : struct_of(type);

        if (built.index == 0 && kind_of_type(type) == TYPE_ARRAY
            && !is_reference_type(module_index, type)
            && (owner->constructs(ir) || owner->has_destructor(ir))) {
            built = ir;
        }

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

    give_back(given, node);
}

// what 'return given' gives back, 'node' the statement or, for a closure of
// one expression (record 0058), the expression itself
void FunctionLowerer::give_back(u32 given, u32 node) {
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
        if (built.index != 0 && (!is_place(given) || converts(given, built))) {
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

        if (!failed) {
            copy_construct(place, slot_type(type), from, here);
        }

        return;
    }

    // a number nothing asked to be anything -- the default an enum writes
    // for a variant's payload, '(0, 0)', which no call typed -- is the type
    // it is written into
    AstNodeKind written = kind_of(given);
    IrValueId value =
        type_at(given) == INVALID_TYPE
                && (written == AST_INTEGER_LITERAL
                    || written == AST_FLOAT_LITERAL
                    || written == AST_CHAR_LITERAL)
            ? literal(given, types->value_of(type))
            : value_of(given);

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
            || kind == AST_LIST || kind == AST_INCLUSIVE_RANGE
            || kind == AST_EXCLUSIVE_RANGE) {
            if (struct_of(type_at(node)).index == 0) {
                not_yet("a literal that builds a class");
                return none;
            }

            return reference_to(node);
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

    // record 0043: a variant of an enum that is only its tag is the tag
    u32 variant_module = 0;
    u32 variant = 0;

    if (variant_named(node, variant_module, variant)) {
        IrTypeId tag = ir_type(type_at(node));

        return failed ? IrValueId{0}
                      : build.constant(tag,
                                       tag_of_variant(variant_module, variant),
                                       at(node));
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

    // record 0041: ':name', ':'name'' and ':"two words"' are one entry per
    // name, the quotes how a name is written and not part of it
    case AST_SYMBOL_LITERAL: {
        std::string text = text_of(node).substr(1);

        if (text.size() >= 2 && (text[0] == '\'' || text[0] == '"')) {
            text = unescape(text.substr(1, text.size() - 2));
        }

        return build.address(owner->symbol(text), at(node));
    }

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
IrValueId FunctionLowerer::literal(u32 node, u32 wanted) {
    u32 type = type_at(node) == INVALID_TYPE ? wanted : type_at(node);
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
        IrObjectId target = function_of(chosen->module,
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
            not_yet("a construction of a union or an enum");
            return none;
        }

        IrValueId place = into;

        if (place.index == 0) {
            place = build.entry_slot(built, at(node));
        }

        // 'T()' is C++'s value-initialisation: a struct or a class with no
        // 'init' is zeroed before it is built, as 'new T' is
        if (chosen->candidate == 0) {
            if (!owner->has_init(built)) {
                zero(place, built, at(node));
            }

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

    // record 0058: a value of type 'A -> R' -- a local, a parameter, a field
    // or anything giving one back -- is called through its pair
    if (kind_of_type(type_at(callee)) == TYPE_FUNCTION
        && (kind_of(name) != AST_IDENTIFIER
            || (SymbolKind) compilation
                       ->get_module(
                           module->get_resolutions()->get(name)->module)
                       ->get_symbols()
                       ->get_candidate(
                           module->get_resolutions()->get(name)->candidate)
                       ->kind
                   != SYMBOL_FUNCTION)) {
        return call_value(node, into);
    }

    if (kind_of(name) != AST_IDENTIFIER) {
        not_yet("a call through '" + text_of(callee) + "'");
        return none;
    }

    Resolution* found = module->get_resolutions()->get(name);
    IrObjectId target = function_of(found->module, found->candidate);

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
    if (shape & SHAPE_METHOD) {
        values.push_back(receiver_of(callee, found->module, found->candidate));
    }

    if (shape & SHAPE_DESTINATION) {
        built = struct_of(signature_of(found->module, found->candidate).back(),
                          found->module);

        if (place.index == 0) {
            place = build.entry_slot(built, at(node));
        }

        values.push_back(place);
    }

    if (failed
        || !arguments_for(found->module, found->candidate, first, values)) {
        return none;
    }

    // 'super.f()' is the base's own, called without dispatch (record 0074)
    bool through_super = (kind_of(callee) == AST_DOT
                          || kind_of(callee) == AST_ARROW)
                      && kind_of(child_of(callee, 0)) == AST_SUPER;
    IrValueId given =
        (shape & SHAPE_METHOD)
            ? call_method(found->module, found->candidate, values,
                          !through_super, at(node))
            : build.call(target, values, at(node));

    if (shape & SHAPE_DESTINATION) {
        if (into.index == 0) {
            add_cleanup(place, built, false, at(node));
        }

        return place;
    }

    return given;
}

// whether a node written where a value goes names a function -- 'f',
// '::f', 'c.add', or a bare method inside its class -- as its candidate
bool FunctionLowerer::names_function(u32 node, u32& in_module,
                                     u32& candidate) {
    u32 name = node;

    while (kind_of(name) == AST_PARENTHESIS) {
        name = child_of(name, 0);
    }

    if (kind_of(name) == AST_DOT || kind_of(name) == AST_ARROW) {
        name = child_of(name, 1);
    } else if (kind_of(name) == AST_SCOPE) {
        u32 first = child_of(name, 0);
        u32 second = first == 0 ? 0
                                : module->get_ast()->get_node(first)->get_sibling();

        name = second == 0 ? first : second;
    }

    if (kind_of(name) == AST_GENERIC_NAME) {
        name = child_of(name, 0);
    }

    if (kind_of(name) != AST_IDENTIFIER) {
        return false;
    }

    Resolution* found = module->get_resolutions()->get(name);

    if (found->candidate == 0
        || (SymbolKind) compilation->get_module(found->module)->get_symbols()
                   ->get_candidate(found->candidate)->kind
               != SYMBOL_FUNCTION) {
        return false;
    }

    in_module = found->module;
    candidate = found->candidate;

    return true;
}

// Record 0058 section 8 and record 0071: a function as a value. A 'def'
// has no environment; a method's is its object -- the left of 'c.add', or
// 'this' for a bare name inside its class -- which the value refers to
void FunctionLowerer::build_function_value(IrValueId place, IrTypeId type,
                                           u32 node, u32 in_module,
                                           u32 candidate) {
    IrTypeId pointer = program->get_types()->pointer(IR_SPACE_GENERIC);
    IrPosition here = at(node);
    IrValueId environment = {0};
    IrTypeId from = {0};
    u32 written = node;

    while (kind_of(written) == AST_PARENTHESIS) {
        written = child_of(written, 0);
    }

    if (owner->holder_of(in_module, candidate) != 0) {
        if (kind_of(written) == AST_DOT || kind_of(written) == AST_ARROW) {
            u32 left = child_of(written, 0);

            if (kind_of(left) == AST_THIS || kind_of(left) == AST_SUPER) {
                environment = build.load(pointer, self, here);
                from = holder;
            } else if (kind_of_type(type_at(left)) == TYPE_POINTER) {
                environment = value_of(left);
                from = element_of(type_at(left));
            } else {
                environment = reference_to(left);
                from = struct_of(type_at(left));
            }
        } else if (self.index != 0) {
            environment = build.load(pointer, self, here);
            from = holder;
        } else {
            not_yet("a method as a value outside its class");
            return;
        }
    } else {
        environment = build.null(here);
    }

    if (failed) {
        return;
    }

    IrObjectId adapter = owner->adapter_of(in_module, candidate, from,
                                           module_index,
                                           module->get_types()->value_of(
                                               type_at(node)));

    if (adapter.index == 0 || owner->get_error().size() > 0) {
        failed = true;
        return;
    }

    build.store(pointer, environment, build.field(type, place, 0, here), here);
    build.store(pointer, build.address(adapter, here),
                build.field(type, place, 1, here), here);
}

// The adapter's body: what it was given after the environment passed on as
// it is -- where a result is built, then each parameter -- to the function,
// or to the method called on the object the environment is
bool FunctionLowerer::lower_adapter(const AdapterJob& job) {
    Module* owning = compilation->get_module(job.module);
    IrPosition here = {job.module, IR_NO_OFFSET};

    begin(job.module, job.function);
    here = at(owning->get_symbols()->get_candidate(job.candidate)->ast_node);

    const IrSignature& taken =
        program->get_signature(program->get_function(job.function).signature);
    IrObjectId target = function_of(job.module, job.candidate);

    if (failed) {
        return false;
    }

    std::vector<IrValueId> values;

    if (job.method) {
        values.push_back(upcast_to(build.parameter(0), job.from,
                                   owner->receiver_type(job.module,
                                                        job.candidate),
                                   here));
    }

    for (u32 p = 1; p < taken.parameters.size(); p++) {
        values.push_back(build.parameter(p));
    }

    IrValueId given = job.method
                          ? call_method(job.module, job.candidate, values, true,
                                        here)
                          : build.call(target, values, here);

    if (failed) {
        return false;
    }

    if (result.index != 0) {
        build.ret(result, given, here);
    } else {
        build.ret(here);
    }

    return !failed;
}

// Record 0058: 'f(a)' with 'f' a value of type 'A -> R', called through
// its function with its environment first -- where an aggregate result is
// built next, as for any call (question 80)
IrValueId FunctionLowerer::call_value(u32 node, IrValueId into) {
    IrValueId none = {0};
    u32 callee = child_of(node, 0);
    u32 arguments = child_of(node, 1);
    u32 first = arguments == 0 ? 0 : child_of(arguments, 0);
    u32 type = module->get_types()->value_of(type_at(callee));
    IrTypeId pair = struct_of(type);
    IrTypeId pointer = program->get_types()->pointer(IR_SPACE_GENERIC);
    bool destination = false;
    u32 signature = owner->signature_of_value(module_index, type, destination);
    IrValueId held = reference_to(callee);

    if (failed || owner->get_error().size() > 0) {
        failed = true;
        return none;
    }

    IrValueId environment =
        build.load(pointer, build.field(pair, held, 0, at(node)), at(node));
    IrValueId function =
        build.load(pointer, build.field(pair, held, 1, at(node)), at(node));
    std::vector<u32> written = module->get_types()->get_arguments(type);
    std::vector<IrValueId> values = {environment};
    IrValueId place = into;
    IrTypeId built = {0};

    if (destination) {
        built = struct_of(written.back());

        if (place.index == 0) {
            place = build.entry_slot(built, at(node));
        }

        values.push_back(place);
    }

    u32 at_parameter = 0;

    if (!arguments_written(module_index, written, first, values, 0xffffffff,
                           at_parameter)) {
        return none;
    }

    IrValueId given = build.call_indirect(signature, function, values,
                                          at(node));

    if (destination) {
        if (into.index == 0) {
            add_cleanup(place, built, false, at(node));
        }

        return place;
    }

    return given;
}

// Record 0058: a closure's value, '{env, call}', where it is written. Its
// environment is a slot of this function, filled here with the address of
// each variable it captures, so what its body changes is the variable
void FunctionLowerer::build_closure(IrValueId place, IrTypeId type, u32 node) {
    IrObjectId function = owner->closure_of(module_index, node, holder);
    IrTypeId pointer = program->get_types()->pointer(IR_SPACE_GENERIC);
    IrPosition here = at(node);

    if (function.index == 0 || owner->get_error().size() > 0) {
        failed = true;
        return;
    }

    const ClosureJob job = owner->closure_job(module_index, node);
    IrValueId environment = {0};

    if (job.environment.index == 0) {
        environment = build.null(here);
    } else {
        environment = build.entry_slot(job.environment, here);

        for (u32 i = 0; i < job.captures.size() && !failed; i++) {
            IrValueId address = address_of_local(job.captures[i], here);

            build.store(pointer, address,
                        build.field(job.environment, environment, i, here),
                        here);
        }

        if (job.uses_self) {
            if (self.index == 0) {
                not_yet("a closure that reads 'this' outside a method");
                return;
            }

            build.store(pointer, build.load(pointer, self, here),
                        build.field(job.environment, environment,
                                    (u32) job.captures.size(), here),
                        here);
        }
    }

    build.store(pointer, environment, build.field(type, place, 0, here), here);
    build.store(pointer, build.address(function, here),
                build.field(type, place, 1, here), here);
}

// where a local or a parameter is: its slot, or what the slot holds for a
// reference and for what was given by address
IrValueId FunctionLowerer::address_of_local(u32 candidate, IrPosition here) {
    std::map<u32, IrValueId>::iterator slot = slots.find(candidate);

    if (slot == slots.end()) {
        not_yet("a closure capturing what has no slot");
        return {0};
    }

    if (is_reference_type(module_index,
                          module->get_symbols()->get_candidate(candidate)->type)
        || indirect.count(candidate) > 0) {
        return build.load(program->get_types()->pointer(IR_SPACE_GENERIC),
                          slot->second, here);
    }

    return slot->second;
}

// A closure's body: its environment first, each capture a local reached
// through the address the environment holds, 'self' too when it reads the
// object; then its parameters, and its body -- one expression given back
bool FunctionLowerer::lower_closure(const ClosureJob& job) {
    AstQuery query;
    IrTypeId pointer = program->get_types()->pointer(IR_SPACE_GENERIC);

    begin(job.module, job.function);
    query.set_module(module);

    u32 type = module->get_types()->value_of(type_at(job.node));
    std::vector<u32> written = module->get_types()->get_arguments(type);
    const IrSignature& taken =
        program->get_signature(program->get_function(job.function).signature);
    IrPosition here = at(job.node);
    u32 first = 1;

    returning = written.size() == 0 ? INVALID_TYPE : written.back();

    if (job.environment.index != 0) {
        IrValueId environment = build.parameter(0);

        for (u32 i = 0; i < job.captures.size(); i++) {
            IrValueId slot = build.entry_slot(pointer, here);

            build.store(pointer,
                        build.load(pointer,
                                   build.field(job.environment, environment, i,
                                               here),
                                   here),
                        slot, here);
            slots[job.captures[i]] = slot;
            indirect.insert(job.captures[i]);
        }

        if (job.uses_self) {
            self = build.entry_slot(pointer, here);
            holder = job.holder;
            build.store(pointer,
                        build.load(pointer,
                                   build.field(job.environment, environment,
                                               (u32) job.captures.size(), here),
                                   here),
                        self, here);
        }
    }

    if (owner->shape_of(job.function) & SHAPE_DESTINATION) {
        destination = build.parameter(first++);
    }

    std::vector<u32> parameters = query.get_closure_parameters(job.node);

    for (u32 i = 0; i < parameters.size(); i++) {
        u32 candidate = module->get_symbols()->candidate_of(parameters[i]);
        IrTypeId one = taken.parameters[first + i];
        IrValueId slot = build.entry_slot(one, at(parameters[i]));
        u32 declared = module->get_symbols()->get_candidate(candidate)->type;

        build.store(one, build.parameter(first + i), slot, at(parameters[i]));
        slots[candidate] = slot;

        if ((struct_of(declared).index != 0
             || kind_of_type(declared) == TYPE_ARRAY)
            && !is_reference_type(module_index, declared)) {
            indirect.insert(candidate);
        }
    }

    u32 block = query.get_block(job.node);
    u32 back = query.get_given_back(job.node);
    bool gives = returning != INVALID_TYPE
              && !(module->get_types()->get_type(returning)->kind
                       == TYPE_BUILTIN
                   && module->get_types()->get_type(returning)->subject
                          == BUILTIN_VOID);

    if (back != 0 && gives) {
        open_scope(false);

        for (u32 before = child_of(block, 0); before != 0 && before != back
             && !failed;
             before = module->get_ast()->get_node(before)->get_sibling()) {
            lower_statement(before);
        }

        if (!failed) {
            open_scope(true);
            give_back(back, back);
            close_scope(here);
        }

        close_scope(here);
    } else {
        lower_block(block);
    }

    if (!failed && !build.ended() && result.index == 0) {
        build.ret(here);
    }

    return !failed;
}

// each argument written into its parameter, by the parameter's type read in
// 'callee_module': a 'T&' given a place, a struct by value a copy the caller
// makes; 'at_parameter' says how many were given
bool FunctionLowerer::arguments_written(u32 callee_module,
                                        const std::vector<u32>& written,
                                        u32 first,
                                        std::vector<IrValueId>& values,
                                        u32 limit, u32& at_parameter) {
    for (u32 one = first; one != 0 && at_parameter < limit;
         one = module->get_ast()->get_node(one)->get_sibling(), at_parameter++) {
        u32 wanted = at_parameter + 1 < written.size() ? written[at_parameter]
                                                       : INVALID_TYPE;
        IrTypeId built = wanted == INVALID_TYPE
                             ? IrTypeId{0}
                             : struct_of(wanted, callee_module);

        // a 'T&' is given a place; a struct by value a copy the caller makes
        // and ends with the statement (question 83, Itanium's rule)
        if (wanted != INVALID_TYPE && is_reference_type(callee_module, wanted)
            && !converts(one, built)) {
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

    return !failed;
}

bool FunctionLowerer::arguments_for(u32 callee_module, u32 candidate,
                                    u32 first, std::vector<IrValueId>& values,
                                    u32 limit) {
    std::vector<u32> written = signature_of(callee_module, candidate);
    u32 at_parameter = 0;

    if (!arguments_written(callee_module, written, first, values, limit,
                           at_parameter)) {
        return false;
    }

    // record 0012: what is left out is the default the parameter declares
    if (at_parameter + 1 < written.size()) {
        u32 function = compilation->get_module(callee_module)->get_symbols()
                           ->get_candidate(candidate)->ast_node;
        AstQuery query;

        query.set_module(compilation->get_module(callee_module));

        std::vector<u32> parameters = query.get_params(function);

        for (; at_parameter + 1 < written.size() && !failed; at_parameter++) {
            u32 wanted = written[at_parameter];
            IrTypeId built = struct_of(wanted, callee_module);

            if (built.index != 0
                && !is_reference_type(callee_module, wanted)) {
                not_yet("a default of a struct given by value");
                return false;
            }

            values.push_back(default_of(callee_module,
                                        parameters[at_parameter],
                                        is_reference_type(callee_module,
                                                          wanted)));
        }
    }

    return !failed;
}

IrValueId FunctionLowerer::default_of(u32 owning, u32 parameter,
                                      bool reference) {
    IrValueId none = {0};
    AstQuery query;

    query.set_module(compilation->get_module(owning));

    u32 given = query.get_binding_expression(parameter);

    if (given == 0) {
        not_yet("a call that leaves out a parameter with no default");
        return none;
    }

    // read in the module that wrote it, and only there: its nodes, types and
    // resolutions are that module's (record 0016)
    u32 kept_index = module_index;
    Module* kept = module;

    module_index = owning;
    module = compilation->get_module(owning);

    IrValueId made = reference ? reference_to(given) : value_of(given);

    module_index = kept_index;
    module = kept;

    return made;
}

// a method's object: 'this' inside one, the left of a '.' or a '->' --
// through it when it is a pointer -- or 'super', the object as its base
IrValueId FunctionLowerer::receiver_of(u32 callee, u32 callee_module,
                                       u32 candidate) {
    IrValueId none = {0};
    u32 declaration = owner->holder_of(callee_module, candidate);
    IrTypeId target = owner->receiver_type(callee_module, candidate);
    IrTypeId pointer = program->get_types()->pointer(IR_SPACE_GENERIC);
    IrValueId object = none;
    IrTypeId from = holder;
    AstNodeKind declared = compilation->get_module(callee_module)->get_ast()
                               ->get_node(declaration)->get_kind();

    // a union's and an enum's methods are a struct's: never virtual, the
    // object given by its address (records 0064 and 0043)
    if (declared != AST_STRUCT && declared != AST_CLASS
        && declared != AST_UNION && declared != AST_ENUM) {
        not_yet("a call of a method of '" + text_of(callee) + "'");
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
// left operand is the object, through it when it is a pointer. 'x in c' is
// 'c.contains(x)', the operands the other way round, and 'not in' its 'not'
IrValueId FunctionLowerer::operator_call(u32 node, IrValueId into) {
    IrValueId none = {0};
    Resolution* chosen = module->get_resolutions()->get(node);
    IrObjectId target = function_of(chosen->module, chosen->candidate);

    if (target.index == 0) {
        not_yet("an operator a class overloads, or a construction");
        return none;
    }

    bool membership = kind_of(node) == AST_IN || kind_of(node) == AST_NOT_IN;
    u32 left = membership ? child_of(node, 1) : child_of(node, 0);
    IrTypeId target_type = owner->receiver_type(chosen->module,
                                                chosen->candidate);
    u8 shape = owner->shape_of(target);
    std::vector<IrValueId> values;
    IrValueId place = into;
    IrTypeId built = {0};

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

    if (shape & SHAPE_DESTINATION) {
        built = struct_of(signature_of(chosen->module, chosen->candidate).back(),
                          chosen->module);

        if (place.index == 0) {
            place = build.entry_slot(built, at(node));
        }

        values.push_back(place);
    }

    if (!arguments_for(chosen->module, chosen->candidate,
                       membership
                           ? child_of(node, 0)
                           : module->get_ast()->get_node(left)->get_sibling(),
                       values, membership ? 1 : 0xffffffff)) {
        return none;
    }

    IrValueId given = call_method(chosen->module, chosen->candidate, values,
                                  true, at(node));

    if (kind_of(node) == AST_NOT_IN) {
        return build.unary(IR_NOT, program->get_types()->scalar(IR_BOOL),
                           given, at(node));
    }

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

    // Hadley 2026-11-03: two characters subtract into an i32, each widened
    // as 'c as i32' widens one, with no sign; and a character moves by a
    // whole number, cut to the character's width, wrapping as any eight bits
    if (kind == AST_MINUS && is_char_type(left_type)
        && is_char_type(right_type)) {
        IrTypeId wide = ir_type(type);

        left = build.convert(IR_ZEXT, wide, left, at(node));
        right = build.convert(IR_ZEXT, wide, right, at(node));

        return build.binary(IR_SUB, wide, left, right, at(node));
    }

    if (is_char_type(left_type) && is_whole_type(right_type)
        && (kind == AST_PLUS || kind == AST_MINUS)) {
        return move_char(left_type, left, right, right_type,
                         kind == AST_MINUS, at(node));
    }

    return apply(kind, type, left, right, at(node));
}

// record 0079: a character moved by a whole number, cut to its eight bits
IrValueId FunctionLowerer::move_char(u32 type, IrValueId character,
                                     IrValueId by, u32 by_type, bool minus,
                                     IrPosition here) {
    IrTypeId narrow = ir_type(type);

    if (ir_type(by_type).index != narrow.index) {
        by = build.convert(IR_TRUNC, narrow, by, here);
    }

    return build.binary(minus ? IR_SUB : IR_ADD, narrow, character, by, here);
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

    if ((kind == AST_POWER || kind == AST_INTEGER_DIVISION)
        && is_float_type(type)) {
        return float_helper(kind, type, left, right, here);
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

// Record 0057's 'a ** b' and 'a // b' over a float, as the C++ path's helpers
// compute them: both sides as an f64, whichever float they are, and the
// answer cut back. '//' divides first, so an f32's quotient is an f64's
IrValueId FunctionLowerer::float_helper(AstNodeKind kind, u32 type,
                                        IrValueId left, IrValueId right,
                                        IrPosition here) {
    IrTypeTable* types = program->get_types();
    IrTypeId wide = types->scalar(IR_F64);
    IrTypeId own = ir_type(type);
    bool narrow = own.index != wide.index;

    if (narrow) {
        left = build.convert(IR_FPEXT, wide, left, here);
        right = build.convert(IR_FPEXT, wide, right, here);
    }

    IrValueId answer =
        kind == AST_POWER
            ? build.call(owner->power_of_floats(), {left, right}, here)
            : build.call(owner->floor_of_float(),
                         {build.binary(IR_FDIV, wide, left, right, here)}, here);

    return narrow ? build.convert(IR_FPTRUNC, own, answer, here) : answer;
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

    // Records 0043 and 0067: two enums that carry something, or two tuples,
    // are equal when they hold the same thing -- read where they are
    if ((kind_of(node) == AST_EQUAL || kind_of(node) == AST_NOT_EQUAL)
        && type != INVALID_TYPE && struct_of(type).index != 0) {
        IrValueId left = reference_to(child_of(node, 0));
        IrValueId right = reference_to(child_of(node, 1));

        if (failed) {
            return none;
        }

        IrValueId same =
            equal_at(module->get_types()->value_of(type_at(child_of(node, 0))),
                     module->get_types()->value_of(type_at(child_of(node, 1))),
                     left, right, at(node));

        if (failed || kind_of(node) == AST_EQUAL) {
            return same;
        }

        return build.unary(IR_NOT, program->get_types()->scalar(IR_BOOL),
                           same, at(node));
    }

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

// Whether the values of Haard type 'type' at two addresses are equal: a
// scalar by its own comparison, a tuple element by element, an enum by its
// tag and then by what the variant of that tag carries -- only that one,
// since the others' bytes are not alive. A class compares by the
// 'operator==' it writes, which a tuple or an enum holding one waits for
IrValueId FunctionLowerer::equal_at(u32 type, u32 other, IrValueId left,
                                    IrValueId right, IrPosition here) {
    IrValueId none = {0};
    TypeTable* types = module->get_types();
    IrTypeTable* ir_types = program->get_types();
    IrTypeId boolean = ir_types->scalar(IR_BOOL);
    IrTypeId pointer = ir_types->pointer(IR_SPACE_GENERIC);

    // each side through what it refers to: a tuple of references compares
    // with one of the values it names (record 0068)
    if (is_reference_type(module_index, type)) {
        return equal_at(types->value_of(type), other,
                        build.load(pointer, left, here), right, here);
    }

    if (is_reference_type(module_index, other)) {
        return equal_at(type, types->value_of(other), left,
                        build.load(pointer, right, here), here);
    }

    IrTypeId built = struct_of(type);

    if (built.index == 0) {
        IrTypeId scalar = ir_type(type);

        if (failed) {
            return none;
        }

        return build.binary(is_float_type(type) ? IR_FEQ : IR_EQ, scalar,
                            build.load(scalar, left, here),
                            build.load(scalar, right, here), here);
    }

    Type* entry = types->get_type(type);

    if (entry->kind == TYPE_TUPLE) {
        std::vector<u32> elements = types->get_arguments(type);
        std::vector<u32> others = types->get_arguments(other);
        IrTypeId theirs = struct_of(other);
        IrValueId all = build.constant(boolean, 1, here);

        for (u32 i = 0; i < elements.size() && !failed; i++) {
            IrValueId one = equal_at(elements[i], others[i],
                                     build.field(built, left, i, here),
                                     build.field(theirs, right, i, here), here);

            if (!failed) {
                all = build.binary(IR_AND, boolean, all, one, here);
            }
        }

        return all;
    }

    Module* holder = compilation->get_module(entry->module);

    // a class or a struct by the 'operator==' it writes (record 0034), the
    // other side given as its parameter takes it
    if ((SymbolKind) holder->get_symbols()->get_candidate(entry->subject)->kind
        != SYMBOL_ENUM) {
        u32 equals = owner->method_named(built, "operator==", 1, 0);

        if (equals == 0) {
            not_yet("comparing a struct that writes no 'operator=='");
            return none;
        }

        u32 owning = ir_types->get_aggregate(built)->module;
        std::vector<u32> signature = signature_of(owning, equals);
        IrValueId other = right;

        if (!is_reference_type(owning, signature[0])) {
            other = build.entry_slot(built, here);
            copy_construct(other, built, right, here);
            add_cleanup(other, built, false, here);
        }

        IrValueId object =
            upcast_to(left, built, owner->receiver_type(owning, equals), here);

        return call_method(owning, equals, {object, other}, true, here);
    }

    IrAggregate* aggregate = ir_types->get_aggregate(built);
    IrTypeId tag = aggregate->fields[0].type;
    IrValueId mine = build.load(tag, build.field(built, left, 0, here), here);
    IrValueId theirs =
        build.load(tag, build.field(built, right, 0, here), here);
    IrValueId answer = build.entry_slot(boolean, here);

    build.store(boolean, build.binary(IR_EQ, tag, mine, theirs, here), answer,
                here);

    if (aggregate->fields.size() < 2) {
        return build.load(boolean, answer, here);
    }

    IrTypeId payload = aggregate->fields[1].type;
    std::vector<IrField> members = ir_types->get_aggregate(payload)->fields;

    for (u32 m = 0; m < members.size() && !failed; m++) {
        u32 candidate = members[m].candidate;
        IrValueId is = build.binary(
            IR_EQ, tag, mine,
            build.constant(tag, tag_of_variant(entry->module, candidate), here),
            here);

        build.begin_if(build.binary(IR_AND, boolean,
                                    build.load(boolean, answer, here), is,
                                    here),
                       here);

        IrTypeId carried = members[m].type;
        std::vector<u32> signature = holder->get_types()->get_arguments(
            holder->get_symbols()->get_candidate(candidate)->type);
        IrValueId mine_inside = build.field(
            payload, build.field(built, left, 1, here), m, here);
        IrValueId theirs_inside = build.field(
            payload, build.field(built, right, 1, here), m, here);
        IrValueId all = build.constant(boolean, 1, here);

        for (u32 i = 0; i + 1 < signature.size() && !failed; i++) {
            u32 element = owner->translate_type(module_index, entry->module,
                                                signature[i]);
            IrValueId one = equal_at(
                element, element,
                build.field(carried, mine_inside, i, here),
                build.field(carried, theirs_inside, i, here), here);

            if (!failed) {
                all = build.binary(IR_AND, boolean, all, one, here);
            }
        }

        build.store(boolean, all, answer, here);
        build.end();
    }

    return build.load(boolean, answer, here);
}

// Record 0067: a tuple assigned element by element, in order. An element
// that is a reference is written through, a struct by its own assignment
// ('&&' by its move), a tuple by these same rules. A written right side is
// built in the value shape first (record 0068), so 'swap = (q, p)' reads
// both before it writes either; a right side that is itself a tuple of
// references is read element by element, as the C++ path reads it
void FunctionLowerer::assign_tuple(u32 to_type, IrValueId to, u32 from_type,
                                   IrValueId from, bool moved,
                                   IrPosition here) {
    TypeTable* types = module->get_types();

    IrTypeId mine = struct_of(to_type);
    IrTypeId theirs = struct_of(from_type);
    std::vector<u32> left = types->get_arguments(to_type);
    std::vector<u32> right = types->get_arguments(from_type);
    IrTypeId pointer = program->get_types()->pointer(IR_SPACE_GENERIC);

    for (u32 i = 0; i < left.size() && !failed; i++) {
        IrValueId into = build.field(mine, to, i, here);
        IrValueId out = build.field(theirs, from, i, here);
        u32 one = left[i];
        u32 two = right[i];

        if (is_reference_type(module_index, one)) {
            into = build.load(pointer, into, here);
            one = types->value_of(one);
        }

        if (is_reference_type(module_index, two)) {
            out = build.load(pointer, out, here);
            two = types->value_of(two);
        }

        IrTypeId built = struct_of(one);

        if (kind_of_type(one) == TYPE_TUPLE) {
            assign_tuple(one, into, two, out, moved, here);
        } else if (built.index == 0) {
            IrTypeId scalar = ir_type(one);

            if (!failed) {
                build.store(scalar, build.load(scalar, out, here), into, here);
            }
        } else if (moved && owner->move_init_of(built) != 0) {
            build.call(owner->structor(STRUCTOR_MOVE_ASSIGN, built, 0),
                       {into, out}, here);
        } else if (owner->declares_copy(built)) {
            build.call(owner->structor(STRUCTOR_ASSIGN, built, 0),
                       {into, out}, here);
        } else {
            assign_value(built, into, out, here);
        }
    }
}

// Record 0067: a tuple moved into 'to', element by element -- a struct by
// the move 'init' it writes, or copied when it writes none, as C++'s
// implicit move of a struct does
void FunctionLowerer::move_tuple(u32 type, IrValueId to, IrValueId from,
                                 IrPosition here) {
    TypeTable* types = module->get_types();
    IrTypeId tuple = struct_of(type);
    std::vector<u32> elements = types->get_arguments(type);

    for (u32 i = 0; i < elements.size() && !failed; i++) {
        IrValueId into = build.field(tuple, to, i, here);
        IrValueId out = build.field(tuple, from, i, here);
        u32 one = elements[i];
        IrTypeId built = is_reference_type(module_index, one)
                             ? IrTypeId{0}
                             : struct_of(one);

        if (kind_of_type(one) == TYPE_TUPLE
            && !is_reference_type(module_index, one)) {
            move_tuple(types->value_of(one), into, out, here);
        } else if (built.index != 0 && owner->move_init_of(built) != 0) {
            construct(into, built, owner->move_init_of(built), {out}, here);
        } else if (built.index != 0) {
            copy_construct(into, built, out, here);
        } else {
            IrTypeId scalar = slot_type(one);

            if (!failed) {
                build.store(scalar, build.load(scalar, out, here), into, here);
            }
        }
    }
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

    // record 0067: a tuple is assigned element by element, through the
    // elements that are references
    if (built.index != 0 && kind == AST_ASSIGNMENT
        && kind_of_type(type) == TYPE_TUPLE) {
        u32 right = child_of(node, 1);
        IrValueId to = place_of(child_of(node, 0));
        IrValueId from = reference_to(right);

        if (failed) {
            return none;
        }

        assign_tuple(module->get_types()->value_of(type), to,
                     module->get_types()->value_of(type_at(right)), from,
                     kind_of(right) == AST_MOVE, at(node));

        return none;
    }

    // record 0031: a struct that says how to copy itself is assigned by
    // destroying and copying; '&&x' by destroying and moving (Hadley,
    // 2026-09-30); one that says nothing a field at a time, as C++'s own
    // '=' does, which for a struct of plain fields is its bytes
    if (built.index != 0 && kind == AST_ASSIGNMENT) {
        u32 right = child_of(node, 1);
        IrValueId to = place_of(child_of(node, 0));
        bool moved = kind_of(right) == AST_MOVE
                  && owner->move_init_of(built) != 0;
        IrValueId from = {0};
        IrTypeId given = struct_of(type_at(right));

        // record 0046: a value of another type, a name holding one included,
        // is built into a temporary by its 'init', and assigned from
        if (converts(right, built)) {
            from = build.entry_slot(built, at(node));
            build_into(from, built, right);
            add_cleanup(from, built, false, at(node));
        } else {
            from = reference_to(right);
        }

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
            assign_value(built, to, from, at(node));
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

    u32 by_type = type_at(child_of(node, 1));
    bool moves = plain == AST_PLUS || plain == AST_MINUS;
    IrValueId made =
        moves && kind_of_type(type) == TYPE_POINTER
            ? step(type, old, right, by_type, plain == AST_MINUS, at(node))
        : moves && is_char_type(type) && is_whole_type(by_type)
            ? move_char(type, old, right, by_type, plain == AST_MINUS, at(node))
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
        not_yet("a 'new' of a union, an enum, a tuple or a function");
        return none;
    }

    IrValueId bytes = build.size(element, usize(), at(node));

    if (kind_of(written) == AST_ARRAY_TYPE) {
        u32 length = child_of(written, 1);
        IrValueId count = value_of(length);

        if (failed) {
            return none;
        }

        IrValueId n = to_width(count, type_at(length), usize(), at(node));

        bytes = build.binary(IR_MUL, usize(), bytes, n, at(node));

        if (!types->is_aggregate(element)
            || (!keeps_count(element) && !owner->constructs(element))) {
            return build.call(owner->allocator(), {bytes}, at(node));
        }

        // the count first, where 'delete[]' finds it, then the elements,
        // each built with nothing, first to last
        IrValueId first = {0};

        if (keeps_count(element)) {
            IrValueId cookie = build.size(usize(), usize(), at(node));
            IrValueId start = build.call(
                owner->allocator(),
                {build.binary(IR_ADD, usize(), bytes, cookie, at(node))},
                at(node));

            build.store(usize(), n, start, at(node));
            first = build.index(usize(), start,
                                build.constant(usize(), 1, at(node)), at(node));
        } else {
            first = build.call(owner->allocator(), {bytes}, at(node));
        }

        IrValueId turn = {0};
        IrValueId each = begin_elements(first, element, n, IR_NO_BOUND, false,
                                        turn, at(node));

        construct_by_default(each, element, at(node));
        end_elements(turn, at(node));

        return first;
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
        not_yet("a 'delete' of a union, an enum, a tuple or a function");
        return none;
    }

    // 'delete[]' ends each element, last to first, by the count kept before
    // them, and frees from the count on
    if (kind_of(node) == AST_DELETE_ARRAY
        && program->get_types()->is_aggregate(element)
        && keeps_count(element)) {
        IrTypeId pointer_type = program->get_types()->pointer(IR_SPACE_GENERIC);

        build.begin_if(build.binary(IR_NE, pointer_type, pointer,
                                    build.null(at(node)), at(node)),
                       at(node));

        IrValueId start = build.index(usize(), pointer,
                                      build.constant(usize(), (u64) -1,
                                                     at(node)),
                                      at(node));
        IrValueId n = build.load(usize(), start, at(node));
        IrValueId turn = {0};
        IrValueId each = begin_elements(pointer, element, n, IR_NO_BOUND, true,
                                        turn, at(node));

        destroy(each, element, at(node));
        end_elements(turn, at(node));
        build.call(owner->deallocator(), {start}, at(node));
        build.end();

        return none;
    }

    // a class ends by its slot 0, which is whatever class it really is
    // (record 0026: the destructor is always virtual)
    if (program->get_types()->get_type(element)->kind == IR_CLASS) {
        IrTypeId top = owner->top_class(element);
        IrObjectId ending = owner->structor(STRUCTOR_DESTROY, top, 0);

        build.begin_if(build.binary(IR_NE,
                                    program->get_types()->pointer(
                                        IR_SPACE_GENERIC),
                                    pointer, build.null(at(node)), at(node)),
                       at(node));
        build.vcall(top, 0, program->get_function(ending).signature,
                    {upcast_to(pointer, element, top, at(node))}, at(node));
        build.end();
    } else if (owner->has_destructor(element)) {
        build.begin_if(build.binary(IR_NE,
                                    program->get_types()->pointer(
                                        IR_SPACE_GENERIC),
                                    pointer, build.null(at(node)), at(node)),
                       at(node));
        destroy(pointer, element, at(node));
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
    // Record 0046: a value that is not one of these is built by the 'init'
    // taking it -- 'by_value(p)' with 'p' a 'char*' and 'String' having an
    // 'init(char*)'. A literal's construction is the typer's, below
    if (converts(node, type)) {
        u32 init = owner->init_from(type, module_index, type_at(node));
        u32 holder_module = program->get_types()->get_aggregate(type)->module;
        std::vector<u32> written = signature_of(holder_module, init);
        IrValueId value = is_reference_type(holder_module, written[0])
                              ? reference_to(node)
                              : value_of(node);

        if (!failed) {
            construct(place, type, init, {value}, at(node));
        }

        return;
    }

    u32 variant_module = 0;
    u32 variant = 0;

    if (variant_named(node, variant_module, variant)) {
        write_variant(place, type, node, variant_module, variant);
        return;
    }

    u32 function_module = 0;
    u32 function = 0;

    if (owner->is_function_value(type)
        && names_function(node, function_module, function)) {
        build_function_value(place, type, node, function_module, function);
        return;
    }

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

        // record 0067: a tuple moves element by element, each by its own
        // move 'init' when it writes one
        if (kind_of_type(type_at(child_of(node, 0))) == TYPE_TUPLE
            && is_place(child_of(node, 0))) {
            IrValueId from = place_of(child_of(node, 0));

            if (!failed) {
                move_tuple(module->get_types()->value_of(
                               type_at(child_of(node, 0))),
                           place, from, at(node));
            }

            return;
        }

        if (moved != 0 && is_place(child_of(node, 0))) {
            IrValueId from = place_of(child_of(node, 0));

            if (!failed) {
                construct(place, type, moved, {from}, at(node));
            }

            return;
        }

        break;
    }

    case AST_ARRAY:
    case AST_LIST:
        build_from_literal(place, type, node);
        return;

    case AST_CLOSURE:
        build_closure(place, type, node);
        return;

    // Record 0067: each element built into its field, as a binding of the
    // element's type would be -- a reference holds the address, a class is
    // built by its 'init' (record 0068 holds a temporary for what is not a
    // place)
    case AST_TUPLE: {
        std::vector<u32> elements =
            module->get_types()->get_arguments(
                module->get_types()->value_of(type_at(node)));
        u32 which = 0;

        for (u32 one = child_of(node, 0); one != 0 && !failed;
             one = module->get_ast()->get_node(one)->get_sibling(), which++) {
            IrValueId into = build.field(type, place, which, at(one));

            initialise(into, elements[which], one, at(one));
        }

        return;
    }

    // record 0037: a string literal a class takes is built by the 'init'
    // the typer chose, given the literal's text
    case AST_STRING_LITERAL: {
        Resolution* chosen = module->get_resolutions()->get(node);

        if (chosen->candidate == 0) {
            break;
        }

        construct(place, type, chosen->candidate, {string_literal(node)},
                  at(node));
        return;
    }

    // Record 0052: a range written as a value is its class built from the
    // start, one past the end -- the inclusive '..' adds the step -- and a
    // step of 1, by the 'init' taking those three
    case AST_INCLUSIVE_RANGE:
    case AST_EXCLUSIVE_RANGE: {
        u32 init = owner->method_named(type, "init", 3, 0);

        if (init == 0) {
            not_yet("a range whose class has no 'init' of three");
            return;
        }

        IrTypeId each = program->get_signature(
                            program->get_function(function_of(
                                program->get_types()->get_aggregate(type)
                                    ->module,
                                init)).signature)
                            .parameters[1];
        IrValueId start = value_of(child_of(node, 0));
        IrValueId stop = value_of(child_of(node, 1));
        IrValueId one = build.constant(each, 1, at(node));

        if (failed) {
            return;
        }

        if (kind_of(node) == AST_INCLUSIVE_RANGE) {
            stop = build.binary(IR_ADD, each, stop, one, at(node));
        }

        construct(place, type, init, {start, stop, one}, at(node));
        return;
    }

    default:
        if (module->get_resolutions()->get(node)->candidate != 0
            && !is_place(node) && kind_of(node) != AST_STRING_LITERAL) {
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

void FunctionLowerer::build_from_literal(IrValueId place, IrTypeId type,
                                         u32 node) {
    Resolution* chosen = module->get_resolutions()->get(node);
    IrTypeTable* types = program->get_types();
    IrTypeId count_type = types->scalar(IR_B32);
    u32 first = child_of(node, 0);
    u32 count = 0;

    for (u32 one = first; one != 0;
         one = module->get_ast()->get_node(one)->get_sibling()) {
        count++;
    }

    // the fixed array, or nothing to point at when it is empty
    IrValueId elements = build.null(at(node));

    if (first != 0) {
        u32 element = type_at(first);
        IrTypeId each = slot_type(element);

        if (failed) {
            return;
        }

        elements = build.entry_slot(types->array(each, count), at(node));

        u64 at_index = 0;

        for (u32 one = first; one != 0 && !failed;
             one = module->get_ast()->get_node(one)->get_sibling()) {
            initialise(build.index(each, elements,
                                   build.constant(usize(), at_index++, at(node)),
                                   at(node)),
                       element, one, at(node));
        }
    }

    std::vector<IrValueId> taken = {elements,
                                    build.constant(count_type, count, at(node))};

    // nothing chosen: the literal is its own class, by its 'init' of two
    if (chosen->candidate == 0) {
        u32 init = owner->method_named(type, "init", 2, 0);

        if (init == 0) {
            not_yet("a literal of a class with no 'init' of a pointer and a "
                    "count");
            return;
        }

        construct(place, type, init, taken, at(node));
        return;
    }

    std::vector<u32> written = signature_of(chosen->module, chosen->candidate);

    if (written.size() == 3) {
        construct(place, type, chosen->candidate, taken, at(node));
        return;
    }

    // one parameter: the value it takes is built first, into a local of its
    // own, as the emitter does
    IrTypeId middle = struct_of(written[0], chosen->module);
    u32 init = middle.index == 0 ? 0 : owner->method_named(middle, "init", 2, 0);

    if (init == 0) {
        not_yet("a literal given to an 'init' of a class with no 'init' of a "
                "pointer and a count");
        return;
    }

    IrValueId held = build.entry_slot(middle, at(node));

    construct(held, middle, init, taken, at(node));
    add_cleanup(held, middle, true, at(node));
    construct(place, type, chosen->candidate, {held}, at(node));
}

// Record 0043: the variant a node names -- 'E.v', a bare 'v', or the callee
// of 'E.V(...)' -- as its module and candidate; false when it names none
bool FunctionLowerer::variant_named(u32 node, u32& in_module,
                                    u32& candidate) {
    u32 name = node;

    while (kind_of(name) == AST_PARENTHESIS) {
        name = child_of(name, 0);
    }

    if (kind_of(name) == AST_CALL) {
        name = child_of(name, 0);
    }

    if (kind_of(name) == AST_DOT) {
        name = child_of(name, 1);
    } else if (kind_of(name) == AST_SCOPE) {
        u32 first = child_of(name, 0);
        u32 second = first == 0 ? 0
                                : module->get_ast()->get_node(first)->get_sibling();

        name = second == 0 ? first : second;
    }

    if (kind_of(name) == AST_GENERIC_NAME) {
        name = child_of(name, 0);
    }

    Resolution* found = module->get_resolutions()->get(name);

    if (found->candidate == 0
        || (SymbolKind) compilation->get_module(found->module)->get_symbols()
                   ->get_candidate(found->candidate)->kind
               != SYMBOL_VARIANT) {
        return false;
    }

    in_module = found->module;
    candidate = found->candidate;

    return true;
}

// the tag of a variant, as its enum's declaration counts it
u32 FunctionLowerer::tag_of_variant(u32 in_module, u32 candidate) {
    Module* owning = compilation->get_module(in_module);
    AstQuery query;

    query.set_module(owning);

    return query.get_tag(owner->enum_of_variant(in_module, candidate),
                         owning->get_symbols()->get_candidate(candidate)
                             ->ast_node);
}

// A variant written into an enum that is a struct, '{tag; payload}': the tag,
// then what it carries into the payload's member for it -- the arguments of
// 'E.V(a, b)', or the default its declaration writes when they are left out
void FunctionLowerer::write_variant(IrValueId place, IrTypeId type, u32 node,
                                    u32 in_module, u32 candidate) {
    IrTypeTable* types = program->get_types();
    IrAggregate* aggregate = types->get_aggregate(type);
    IrPosition here = at(node);
    IrTypeId tag = aggregate->fields[0].type;

    build.store(tag, build.constant(tag, tag_of_variant(in_module, candidate),
                                    here),
                build.field(type, place, 0, here), here);

    Module* owning = compilation->get_module(in_module);
    u32 variant = owning->get_symbols()->get_candidate(candidate)->ast_node;
    AstQuery query;

    query.set_module(owning);

    if (query.get_written_type(variant) == 0 || aggregate->fields.size() < 2) {
        return;
    }

    IrTypeId payload = aggregate->fields[1].type;
    IrAggregate* members = types->get_aggregate(payload);
    u32 member = 0;

    while (member < members->fields.size()
           && members->fields[member].candidate != candidate) {
        member++;
    }

    if (member == members->fields.size()) {
        not_yet("a variant its enum does not hold");
        return;
    }

    IrTypeId carried = members->fields[member].type;
    IrValueId inside = build.field(
        payload, build.field(type, place, 1, here), member, here);

    // the constructor's signature, the enum itself last (record 0016)
    std::vector<u32> signature = owning->get_types()->get_arguments(
        owning->get_symbols()->get_candidate(candidate)->type);
    std::vector<u32> given;

    if (kind_of(node) == AST_CALL && child_of(node, 1) != 0) {
        for (u32 one = child_of(child_of(node, 1), 0); one != 0;
             one = module->get_ast()->get_node(one)->get_sibling()) {
            given.push_back(one);
        }
    }

    for (u32 i = 0; i < given.size() && !failed; i++) {
        initialise(build.field(carried, inside, i, here),
                   owner->translate_type(module_index, in_module, signature[i]),
                   given[i], here);
    }

    if (given.size() + 1 >= signature.size() || failed) {
        return;
    }

    // the default, read in the module that wrote it (record 0016): a tuple
    // for a payload of several, one value for a payload of one
    u32 written = query.get_binding_expression(variant);

    if (written == 0) {
        not_yet("a variant built with less than it carries");
        return;
    }

    std::vector<u32> defaults;

    if (owning->get_ast()->get_node(written)->get_kind() == AST_TUPLE) {
        for (u32 one = owning->get_ast()->get_node(written)->get_children();
             one != 0; one = owning->get_ast()->get_node(one)->get_sibling()) {
            defaults.push_back(one);
        }
    } else {
        defaults.push_back(written);
    }

    u32 kept_index = module_index;
    Module* kept = module;

    module_index = in_module;
    module = owning;

    for (u32 i = (u32) given.size(); i + 1 < signature.size() && !failed;
         i++) {
        initialise(build.field(carried, inside, i, here), signature[i],
                   defaults[i], here);
    }

    module_index = kept_index;
    module = kept;
}

// Record 0043: the tag copied, then only the member it says is alive,
// field by field by each field's own copy
void FunctionLowerer::copy_alive(IrTypeId type, IrValueId object,
                                 IrValueId other, IrPosition here) {
    IrTypeTable* types = program->get_types();
    IrAggregate* aggregate = types->get_aggregate(type);
    IrTypeId tag = aggregate->fields[0].type;
    IrTypeId payload = aggregate->fields[1].type;
    IrValueId which = build.load(tag, build.field(type, other, 0, here), here);
    std::vector<IrField> members = types->get_aggregate(payload)->fields;

    build.store(tag, which, build.field(type, object, 0, here), here);

    for (u32 m = 0; m < members.size() && !failed; m++) {
        IrTypeId carried = members[m].type;
        std::vector<IrField> fields = types->get_aggregate(carried)->fields;

        build.begin_if(build.binary(IR_EQ, tag, which,
                                    build.constant(tag,
                                                   tag_of_variant(
                                                       aggregate->module,
                                                       members[m].candidate),
                                                   here),
                                    here),
                       here);

        IrValueId to = build.field(
            payload, build.field(type, object, 1, here), m, here);
        IrValueId from = build.field(
            payload, build.field(type, other, 1, here), m, here);

        for (u32 i = 0; i < fields.size() && !failed; i++) {
            IrValueId into = build.field(carried, to, i, here);
            IrValueId out = build.field(carried, from, i, here);
            IrTypeId one = fields[i].type;

            if (types->is_aggregate(one) || types->get_type(one)->kind == IR_ARRAY) {
                copy_construct(into, one, out, here);
            } else {
                build.store(one, build.load(one, out, here), into, here);
            }
        }

        build.end();
    }
}

// and only the member alive ended, its fields last to first
void FunctionLowerer::destroy_alive(IrTypeId type, IrValueId object,
                                    IrPosition here) {
    IrTypeTable* types = program->get_types();
    IrAggregate* aggregate = types->get_aggregate(type);
    IrTypeId tag = aggregate->fields[0].type;
    IrTypeId payload = aggregate->fields[1].type;
    IrValueId which = build.load(tag, build.field(type, object, 0, here), here);
    std::vector<IrField> members = types->get_aggregate(payload)->fields;

    for (u32 m = 0; m < members.size() && !failed; m++) {
        IrTypeId carried = members[m].type;
        std::vector<IrField> fields = types->get_aggregate(carried)->fields;
        bool ends = false;

        for (const IrField& field : fields) {
            ends = ends || owner->has_destructor(field.type);
        }

        if (!ends) {
            continue;
        }

        build.begin_if(build.binary(IR_EQ, tag, which,
                                    build.constant(tag,
                                                   tag_of_variant(
                                                       aggregate->module,
                                                       members[m].candidate),
                                                   here),
                                    here),
                       here);

        IrValueId inside = build.field(
            payload, build.field(type, object, 1, here), m, here);

        for (u32 i = fields.size(); i > 0 && !failed; i--) {
            if (owner->has_destructor(fields[i - 1].type)) {
                destroy(build.field(carried, inside, i - 1, here),
                        fields[i - 1].type, here);
            }
        }

        build.end();
    }
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
            IrValueId turn = {0};
            IrValueId each = begin_elements(place, type, false, turn, here);

            construct_by_default(each, types->get_type(type)->element, here);
            end_elements(turn, here);
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
    for (u32 member : aggregate->declaration == 0
                          ? std::vector<u32>()
                          : query.get_members(aggregate->declaration)) {
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
            u32 candidate = owning->get_symbols()->candidate_of(member);
            std::vector<IrValueId> values;

            if (arguments_for(aggregate->module, candidate, 0, values)) {
                construct(place, type, candidate, values, here);
            }

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
        IrTypeId element = program->get_types()->get_type(type)->element;
        IrValueId turn = {0};
        IrValueId each = begin_elements(place, type, false, turn, here);
        IrValueId source = build.index(element, from,
                                       build.load(usize(), turn, here), here);

        copy_construct(each, element, source, here);
        end_elements(turn, here);
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
        assign_value(fields[i].type, into, out_of, here);
    }
}

void FunctionLowerer::assign_value(IrTypeId type, IrValueId to, IrValueId from,
                                   IrPosition here) {
    IrTypeTable* types = program->get_types();

    if (types->get_type(type)->kind == IR_CLASS) {
        copy_fields(type, to, from, here);
    } else if (types->is_aggregate(type)
               || types->get_type(type)->kind == IR_ARRAY) {
        build.copy(type, to, from, here);
    } else {
        build.store(type, build.load(type, from, here), to, here);
    }
}

void FunctionLowerer::store_vptr(IrValueId object, IrTypeId type,
                                 IrPosition here) {
    IrTypeId pointer = program->get_types()->pointer(IR_SPACE_GENERIC);
    IrValueId table = build.address(owner->vtable_of(type), here);

    build.store(pointer,
                build.index(pointer, table, build.constant(usize(), 1, here),
                            here),
                object, here);
}

void FunctionLowerer::destroy(IrValueId place, IrTypeId type, IrPosition here) {
    if (program->get_types()->get_type(type)->kind == IR_ARRAY) {
        IrValueId turn = {0};
        IrValueId each = begin_elements(place, type, true, turn, here);

        destroy(each, program->get_types()->get_type(type)->element, here);
        end_elements(turn, here);
        return;
    }

    if (program->get_types()->get_type(type)->kind == IR_CLASS) {
        place = upcast_to(place, type, owner->top_class(type), here);
    }

    build.call(owner->structor(STRUCTOR_DESTROY, type, 0), {place}, here);
}

IrValueId FunctionLowerer::call_method(u32 callee_module, u32 candidate,
                                       const std::vector<IrValueId>& values,
                                       bool virtually, IrPosition here) {
    IrObjectId target = function_of(callee_module, candidate);
    i32 slot = owner->slot_of_method(callee_module, candidate);

    if (failed) {
        return {0};
    }

    if (!virtually || slot < 0) {
        return build.call(target, values, here);
    }

    IrTypeId receiver = owner->receiver_type(callee_module, candidate);

    // question 89: every vtable with this slot needs its function now
    owner->use_slot(receiver, (u32) slot);

    return build.vcall(receiver, (u32) slot,
                       program->get_function(target).signature, values, here);
}

// the owner's, which declares a function the first time one is asked for;
// a declaration that could not be made stops this body too
IrObjectId FunctionLowerer::function_of(u32 callee_module, u32 candidate) {
    IrObjectId found = owner->function_of(callee_module, candidate);

    if (owner->get_error().size() > 0) {
        failed = true;
    }

    return found;
}

// 'turn' counts up from 0 whichever way the elements are walked, and its
// bound is the array's length
IrValueId FunctionLowerer::begin_elements(IrValueId place, IrTypeId array,
                                          bool backwards, IrValueId& turn,
                                          IrPosition here) {
    const IrType* entry = program->get_types()->get_type(array);

    return begin_elements(place, entry->element,
                          build.constant(usize(), entry->subject, here),
                          entry->subject, backwards, turn, here);
}

IrValueId FunctionLowerer::begin_elements(IrValueId place, IrTypeId element,
                                          IrValueId length, u32 bound,
                                          bool backwards, IrValueId& turn,
                                          IrPosition here) {
    turn = build.entry_slot(usize(), here);
    build.store(usize(), build.constant(usize(), 0, here), turn, here);

    IrInstId loop = build.begin_loop(bound, here);
    IrValueId i = build.load(usize(), turn, here);

    build.begin_if(build.binary(IR_UGE, usize(), i, length, here), here);
    build.break_to(loop, here);
    build.end();

    if (backwards) {
        i = build.binary(IR_SUB, usize(),
                         build.binary(IR_SUB, usize(), length, i, here),
                         build.constant(usize(), 1, here), here);
    }

    return build.index(element, place, i, here);
}

bool FunctionLowerer::keeps_count(IrTypeId element) {
    return program->get_types()->get_type(element)->kind == IR_CLASS
        || owner->has_destructor(element);
}

void FunctionLowerer::end_elements(IrValueId turn, IrPosition here) {
    IrValueId i = build.load(usize(), turn, here);

    build.store(usize(),
                build.binary(IR_ADD, usize(), i,
                             build.constant(usize(), 1, here), here),
                turn, here);
    build.end();
}

void FunctionLowerer::zero(IrValueId place, IrTypeId type, IrPosition here) {
    IrTypeTable* types = program->get_types();
    const IrType* entry = types->get_type(type);

    if (entry->kind == IR_ARRAY) {
        IrTypeId element = entry->element;
        IrValueId turn = {0};
        IrValueId each = begin_elements(place, type, false, turn, here);

        zero(each, element, here);
        end_elements(turn, here);
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

    // record 0067: a tuple's '[n]' is its field n, n written as a number; an
    // element that is a reference holds the address of what it names
    if (kind == TYPE_TUPLE) {
        TypeTable* types = module->get_types();
        u32 position = 0;

        for (char digit : text_of(which)) {
            if (digit >= '0' && digit <= '9') {
                position = position * 10 + (u32) (digit - '0');
            }
        }

        IrTypeId tuple = struct_of(type);
        IrValueId object = reference_to(base);

        if (failed) {
            return none;
        }

        IrValueId field = build.field(tuple, object, position, at(node));
        u32 element =
            types->get_arguments(types->value_of(type))[position];

        if (is_reference_type(module_index, element)) {
            return build.load(program->get_types()->pointer(IR_SPACE_GENERIC),
                              field, at(node));
        }

        return field;
    }

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
    u32 variant_module = 0;
    u32 variant = 0;

    if (variant_named(node, variant_module, variant)) {
        return false;
    }

    if (kind_of_type(type_at(node)) == TYPE_FUNCTION
        && names_function(node, variant_module, variant)) {
        return false;
    }

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
        || function_of(found->module, found->candidate).index == 0) {
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
        not_yet("a value of a union, an enum or a function ("
                + (ir.index == 0 ? std::string("no type")
                                 : program->get_types()->name_of(ir))
                + ")");
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

    // a class, a struct, a tuple -- the struct of its shape (record 0067)
    // -- and a function value, '{env, call}' (record 0058)
    if (types->get_type(value)->kind != TYPE_NAMED
        && types->get_type(value)->kind != TYPE_TUPLE
        && types->get_type(value)->kind != TYPE_FUNCTION) {
        return none;
    }

    IrTypeId ir = owner->type_of(owning, value);

    u8 kind = ir.index == 0 ? IR_TYPE_NONE
                            : program->get_types()->get_type(ir)->kind;

    return (kind == IR_STRUCT || kind == IR_CLASS || kind == IR_UNION)
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

// record 0046: whether 'node' reaches the struct 'built' by an 'init' that
// takes it, rather than being one already
bool FunctionLowerer::converts(u32 node, IrTypeId built) {
    return built.index != 0 && struct_of(type_at(node)).index == 0
        && owner->init_from(built, module_index, type_at(node)) != 0;
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

bool FunctionLowerer::is_char_type(u32 type) {
    Type* entry = module->get_types()->get_type(module->get_types()->value_of(type));

    return entry->kind == TYPE_BUILTIN && entry->subject == BUILTIN_CHAR;
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
