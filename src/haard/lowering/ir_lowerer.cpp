#include <haard/lowering/ir_lowerer.h>

#include <haard/ir/ir_flattener.h>
#include <haard/ir/ir_verifier.h>
#include <haard/lowering/function_lowerer.h>

using namespace haard;

IrLowerer::IrLowerer() {
    compilation = nullptr;
    target = nullptr;
    program = nullptr;
    alloc_native.index = 0;
    free_native.index = 0;
}

void IrLowerer::set_compilation(Compilation* compilation) {
    this->compilation = compilation;
}

void IrLowerer::set_target(const Target* target) {
    this->target = target;
}

void IrLowerer::set_program(IrProgram* program) {
    this->program = program;
}

bool IrLowerer::lower() {
    error.clear();
    translator.set_compilation(compilation);
    translator.set_target(target);
    translator.set_types(program->get_types());

    // every class translated before the vtables are worked out
    if (!translator.translate_program()) {
        fail("not lowered yet: " + translator.get_error());
        return false;
    }

    program->get_classes()->set_compilation(compilation);
    program->get_classes()->build();

    return declare_functions() && declare_globals() && lower_bodies()
        && lower_globals() && lower_structors() && verify("lowering");
}

bool IrLowerer::finish() {
    IrFlattener flattener;

    flattener.set_program(program);

    for (u32 i = 1; i < program->get_object_count(); i++) {
        IrObject& object = program->get_object({i});

        if (object.kind == IR_OBJECT_FUNCTION && !object.native) {
            flattener.flatten({i});
        }
    }

    return verify("flatten");
}

const std::string& IrLowerer::get_error() {
    return error;
}

Compilation* IrLowerer::get_compilation() {
    return compilation;
}

IrTypeTranslator* IrLowerer::get_translator() {
    return &translator;
}

const Target* IrLowerer::get_target() {
    return target;
}

IrObjectId IrLowerer::global_of(u32 module, u32 candidate) {
    std::map<std::pair<u32, u32>, u32>::iterator found =
        globals.find(std::make_pair(module, candidate));
    IrObjectId id = {found == globals.end() ? 0 : found->second};

    return id;
}

IrObjectId IrLowerer::text(const std::string& bytes) {
    std::map<std::string, u32>::iterator found = texts.find(bytes);

    if (found != texts.end()) {
        return {found->second};
    }

    IrObjectId made =
        program->add_text("text." + std::to_string(texts.size()), bytes);

    texts[bytes] = made.index;

    return made;
}

IrObjectId IrLowerer::allocator() {
    if (alloc_native.index == 0) {
        IrTypeTable* types = program->get_types();

        alloc_native = program->add_native(
            "__alloc",
            program->signature({types->integer(target->pointer_size)},
                               types->pointer(IR_SPACE_GENERIC)));
    }

    return alloc_native;
}

IrObjectId IrLowerer::deallocator() {
    if (free_native.index == 0) {
        free_native = program->add_native(
            "__free",
            program->signature(
                {program->get_types()->pointer(IR_SPACE_GENERIC)}, {0}));
    }

    return free_native;
}

IrObjectId IrLowerer::function_of(u32 module, u32 candidate) {
    std::map<std::pair<u32, u32>, u32>::iterator found =
        functions.find(std::make_pair(module, candidate));
    IrObjectId id = {found == functions.end() ? 0 : found->second};

    return id;
}

IrTypeId IrLowerer::type_of(u32 module, u32 type) {
    return translator.translate(module, type);
}

void IrLowerer::fail(const std::string& message) {
    if (error.size() == 0) {
        error = message;
    }
}

// An object for every function first, so a call can name one declared
// further down or in another module. A function of 'std.low_io' is a native
// (record 0030), and the entry module's 'main' is the one called 'main'. A
// struct's and a class's methods are functions too, named after it
bool IrLowerer::declare_functions() {
    for (u32 i = 0; i < compilation->get_module_count(); i++) {
        Module* module = compilation->get_module(i);
        AstQuery query;

        query.set_module(module);

        for (u32 node : query.get_functions()) {
            if (!is_generic(i, node) && !declare_function(i, node, 0)) {
                return false;
            }
        }

        for (u32 declaration : query.get_declarations()) {
            AstNodeKind kind =
                module->get_ast()->get_node(declaration)->get_kind();

            if ((kind != AST_STRUCT && kind != AST_CLASS)
                || is_generic(i, declaration)) {
                continue;
            }

            for (u32 member : query.get_members(declaration)) {
                if (module->get_ast()->get_node(member)->get_kind()
                        == AST_FUNCTION
                    && !is_generic(i, member)
                    && !declare_function(i, member, declaration)) {
                    return false;
                }
            }
        }
    }

    return true;
}

// Question 80: an aggregate is never a value. One given by value is a copy
// the caller made and passes by its address; one given back is built where
// the caller says, a pointer passed before the parameters. A method's object
// comes first of all, where a 'vcall' finds it (question 83)
bool IrLowerer::declare_function(u32 i, u32 node, u32 holder) {
    Module* module = compilation->get_module(i);
    SymbolTable* symbols = module->get_symbols();
    IrTypeTable* types = program->get_types();
    IrTypeId pointer = types->pointer(IR_SPACE_GENERIC);
    u32 candidate = symbols->candidate_of(node);
    AstQuery query;

    query.set_module(module);

    if (candidate == 0) {
        return true;
    }

    std::vector<u32> written = module->get_types()->get_arguments(
        symbols->get_candidate(candidate)->type);
    std::vector<IrTypeId> parameters;
    IrTypeId result = written.size() == 0 ? IrTypeId{0}
                                          : type_of(i, written.back());
    u8 shape = 0;

    if (holder != 0) {
        parameters.push_back(pointer);
        shape |= SHAPE_METHOD;
    }

    if (types->is_aggregate(result)) {
        if (!lowerable(result)) {
            fail("not lowered yet: a function giving back a union, an enum "
                 "or a tuple");
            return false;
        }

        parameters.push_back(pointer);
        result.index = 0;
        shape |= SHAPE_DESTINATION;
    }

    for (u32 p = 0; p + 1 < written.size(); p++) {
        IrTypeId one = type_of(i, written[p]);

        if (one.index == 0 || !lowerable(one)) {
            fail("not lowered yet: a parameter of a union, an enum, a tuple "
                 "or a function");
            return false;
        }

        parameters.push_back(types->is_aggregate(one) ? pointer : one);
    }

    std::string name = query.get_declaration_name(node);

    // record 0051's third entry point builds a List of Strings, the
    // standard library's -- step 6
    if (i == 0 && holder == 0 && name == "main" && written.size() == 2) {
        fail("not lowered yet: a 'main' taking its arguments as a list");
        return false;
    }

    if (holder != 0) {
        name = types->get_aggregate(translator.translate_declaration(i, holder))
                   ->name
             + "." + name;
    } else if (!(i == 0 && name == "main")) {
        name = module->get_name().size() > 0 ? module->get_name() + "." + name
                                             : name;
    }

    u32 signature = program->signature(parameters, result);
    IrObjectId made =
        module->get_name() == "std.low_io" && holder == 0
            ? program->add_native(query.get_declaration_name(node), signature)
            : program->add_function(unique(name), signature,
                                    IR_FORM_STRUCTURED);

    functions[std::make_pair(i, candidate)] = made.index;
    shapes[made.index] = shape;

    if (!program->get_object(made).native) {
        bodies.push_back(std::make_pair(i, node));
    }

    return true;
}

// Record 0026's rule holds for a global too: a type that is not a scalar or
// an array of them waits for step 3, with its 'init'
bool IrLowerer::declare_globals() {
    for (u32 i = 0; i < compilation->get_module_count(); i++) {
        Module* module = compilation->get_module(i);
        SymbolTable* symbols = module->get_symbols();
        AstQuery query;

        query.set_module(module);

        for (u32 statement : query.get_global_variables()) {
            for (u32 binding = module->get_ast()->get_node(statement)
                                   ->get_children();
                 binding != 0;
                 binding = module->get_ast()->get_node(binding)->get_sibling()) {
                u32 candidate = symbols->candidate_of(binding);

                if (candidate == 0) {
                    fail("not lowered yet: a global that takes a value apart");
                    return false;
                }

                IrTypeId type =
                    type_of(i, symbols->get_candidate(candidate)->type);

                if (type.index == 0 || program->get_types()->is_aggregate(type)) {
                    fail("not lowered yet: a global of a type that is not a "
                         "scalar or an array");
                    return false;
                }

                // the binding's name is the identifier under its
                // AST_BINDING_NAME
                std::string name;

                for (u32 part = module->get_ast()->get_node(binding)
                                    ->get_children();
                     part != 0;
                     part = module->get_ast()->get_node(part)->get_sibling()) {
                    AstNode* node = module->get_ast()->get_node(part);

                    if (node->get_kind() == AST_BINDING_NAME
                        && node->get_children() != 0) {
                        name = std::string(module->get_token_value(
                            module->get_ast()
                                ->get_node(node->get_children())
                                ->get_token()));
                    }
                }

                if (module->get_name().size() > 0) {
                    name = module->get_name() + "." + name;
                }

                globals[std::make_pair(i, candidate)] =
                    program->add_zeroed(unique(name), type).index;
            }
        }
    }

    return true;
}

bool IrLowerer::lower_bodies() {
    for (const std::pair<u32, u32>& one : bodies) {
        Module* module = compilation->get_module(one.first);
        u32 candidate = module->get_symbols()->candidate_of(one.second);
        FunctionLowerer body(this, program);

        if (!body.lower(one.first, one.second,
                        function_of(one.first, candidate))) {
            return false;
        }
    }

    return true;
}

bool IrLowerer::lower_globals() {
    for (u32 i = 0; i < compilation->get_module_count(); i++) {
        Module* module = compilation->get_module(i);
        AstQuery query;

        query.set_module(module);

        if (query.get_global_variables().size() == 0) {
            continue;
        }

        std::string name = module->get_name().size() > 0
                               ? module->get_name() + ".globals"
                               : "globals";
        IrObjectId made = program->add_function(
            unique(name), program->signature({}, {0}), IR_FORM_STRUCTURED);
        FunctionLowerer body(this, program);

        program->add_initialiser(made);

        if (!body.lower_globals(i, made)) {
            return false;
        }
    }

    return true;
}

std::string IrLowerer::unique(const std::string& name) {
    u32 taken = names[name]++;

    return taken > 0 ? name + "#" + std::to_string(taken) : name;
}

// a constructor of each struct is asked for while a body is lowered, and
// lowering one may ask for more: the list grows while it is walked
bool IrLowerer::lower_structors() {
    for (u32 i = 0; i < structors.size(); i++) {
        StructorJob job = structors[i];
        FunctionLowerer body(this, program);

        if (!body.lower_structor(job)) {
            return false;
        }
    }

    return true;
}

u8 IrLowerer::shape_of(IrObjectId function) {
    std::map<u32, u8>::iterator found = shapes.find(function.index);

    return found == shapes.end() ? 0 : found->second;
}

u32 IrLowerer::holder_of(u32 module_index, u32 candidate) {
    SymbolTable* table = compilation->get_module(module_index)->get_symbols();
    u32 inside = table->scope_owned_by(table->get_candidate(candidate)->ast_node);

    if (inside == 0) {
        return 0;
    }

    u32 around = table->get_scope(inside)->parent;

    return around == 0 ? 0 : table->get_scope(around)->owner;
}

IrTypeId IrLowerer::type_of_declaration(u32 module, u32 declaration) {
    return translator.translate_declaration(module, declaration);
}

bool IrLowerer::lowerable(IrTypeId type) {
    IrTypeTable* types = program->get_types();
    const IrType* entry = types->get_type(type);

    if (entry->kind == IR_ARRAY) {
        return lowerable(entry->element);
    }

    if (!types->is_aggregate(type)) {
        return true;
    }

    IrAggregate* aggregate = types->get_aggregate(type);

    AstNodeKind declared =
        aggregate->declaration == 0
            ? AST_UNKNOWN
            : compilation->get_module(aggregate->module)->get_ast()
                  ->get_node(aggregate->declaration)->get_kind();

    if (!(entry->kind == IR_STRUCT && declared == AST_STRUCT)
        && !(entry->kind == IR_CLASS && declared == AST_CLASS)) {
        return false;
    }

    if (aggregate->base.index != 0 && !lowerable(aggregate->base)) {
        return false;
    }

    for (const IrField& field : aggregate->fields) {
        if (!lowerable(field.type)) {
            return false;
        }
    }

    return true;
}

IrObjectId IrLowerer::structor(StructorKind kind, IrTypeId type, u32 init) {
    std::vector<u32> key = {(u32) kind, type.index, init};
    std::map<std::vector<u32>, u32>::iterator found = made_structors.find(key);

    if (found != made_structors.end()) {
        return {found->second};
    }

    IrTypeTable* types = program->get_types();
    IrTypeId pointer = types->pointer(IR_SPACE_GENERIC);
    IrAggregate* aggregate = types->get_aggregate(type);
    std::string name = aggregate->name;
    std::string last = name.substr(name.rfind('.') == std::string::npos
                                       ? 0
                                       : name.rfind('.') + 1);
    std::vector<IrTypeId> parameters = {pointer};

    switch (kind) {
    case STRUCTOR_CONSTRUCT: {
        name += "." + last;

        // the 'init''s own parameters follow the object
        if (init != 0) {
            const IrSignature& taken = program->get_signature(
                program->get_function(function_of(aggregate->module, init))
                    .signature);

            for (u32 p = 1; p < taken.parameters.size(); p++) {
                parameters.push_back(taken.parameters[p]);
            }
        }

        break;
    }

    case STRUCTOR_COPY:
        name += "." + last + "(copy)";
        parameters.push_back(pointer);
        break;

    case STRUCTOR_DESTROY:
        name += ".~" + last;
        break;

    case STRUCTOR_ASSIGN:
        name += ".assign";
        parameters.push_back(pointer);
        break;

    default:
        name += ".move_assign";
        parameters.push_back(pointer);
        break;
    }

    IrObjectId made = program->add_function(
        unique(name), program->signature(parameters, {0}), IR_FORM_STRUCTURED);
    StructorJob job;

    job.kind = kind;
    job.type = type;
    job.init = init;
    job.function = made;
    made_structors[key] = made.index;
    structor_index[made.index] = (u32) structors.size();
    structors.push_back(job);

    return made;
}

const StructorJob& IrLowerer::get_structor(IrObjectId function) {
    return structors[structor_index[function.index]];
}

// A method of the struct by name and number of parameters, and for an 'init'
// of one parameter, what that parameter is: 1 one of this struct, by value
// or by 'T&'; 2 a 'T&&' of it. Whichever parameter count, 'kind' 0 asks
// nothing more
u32 IrLowerer::method_named(IrTypeId type, const std::string& name,
                            u32 parameters, u8 kind) {
    IrAggregate* aggregate = program->get_types()->get_aggregate(type);
    Module* module = compilation->get_module(aggregate->module);
    SymbolTable* table = module->get_symbols();
    TypeTable* types = module->get_types();
    AstQuery query;

    query.set_module(module);

    for (u32 member : query.get_members(aggregate->declaration)) {
        if (module->get_ast()->get_node(member)->get_kind() != AST_FUNCTION
            || query.get_declaration_name(member) != name) {
            continue;
        }

        u32 candidate = table->candidate_of(member);

        if (candidate == 0) {
            continue;
        }

        std::vector<u32> written =
            types->get_arguments(table->get_candidate(candidate)->type);

        if (written.size() != parameters + 1) {
            continue;
        }

        if (kind == 0) {
            return candidate;
        }

        u32 raw = written[0];
        bool moved = types->get_type(raw)->kind == TYPE_MOVE_REFERENCE;
        Type* inside = types->get_type(types->value_of(raw));
        bool mine = inside->kind == TYPE_NAMED
                 && inside->module == aggregate->module
                 && inside->subject
                        == table->candidate_of(aggregate->declaration);

        if (mine && moved == (kind == 2)) {
            return candidate;
        }
    }

    return 0;
}

u32 IrLowerer::destroy_of(IrTypeId type) {
    return method_named(type, "destroy", 0, 0);
}

u32 IrLowerer::copy_init_of(IrTypeId type) {
    return method_named(type, "init", 1, 1);
}

u32 IrLowerer::move_init_of(IrTypeId type) {
    return method_named(type, "init", 1, 2);
}

u32 IrLowerer::empty_init_of(IrTypeId type) {
    return method_named(type, "init", 0, 0);
}

bool IrLowerer::has_init(IrTypeId type) {
    IrAggregate* aggregate = program->get_types()->get_aggregate(type);
    Module* module = compilation->get_module(aggregate->module);
    AstQuery query;

    query.set_module(module);

    for (u32 member : query.get_members(aggregate->declaration)) {
        if (module->get_ast()->get_node(member)->get_kind() == AST_FUNCTION
            && query.get_declaration_name(member) == "init") {
            return true;
        }
    }

    return false;
}

bool IrLowerer::has_destructor(IrTypeId type) {
    IrTypeTable* types = program->get_types();

    if (types->get_type(type)->kind == IR_ARRAY) {
        return has_destructor(types->get_type(type)->element);
    }

    if (!types->is_aggregate(type) || types->get_type(type)->kind == IR_UNION) {
        return false;
    }

    IrAggregate* aggregate = types->get_aggregate(type);

    if (destroy_of(type) != 0
        || (aggregate->base.index != 0 && has_destructor(aggregate->base))) {
        return true;
    }

    for (const IrField& field : aggregate->fields) {
        if (has_destructor(field.type)) {
            return true;
        }
    }

    return false;
}

// a class always is: its copy writes its own vptr, which the bytes of what
// it is copied from -- perhaps a class derived from it -- would not
bool IrLowerer::copies_by_init(IrTypeId type) {
    IrTypeTable* types = program->get_types();

    if (types->get_type(type)->kind == IR_ARRAY) {
        return copies_by_init(types->get_type(type)->element);
    }

    if (types->get_type(type)->kind == IR_CLASS) {
        return true;
    }

    if (types->get_type(type)->kind != IR_STRUCT) {
        return false;
    }

    IrAggregate* aggregate = types->get_aggregate(type);

    if (copy_init_of(type) != 0
        || (aggregate->base.index != 0 && copies_by_init(aggregate->base))) {
        return true;
    }

    for (const IrField& field : aggregate->fields) {
        if (copies_by_init(field.type)) {
            return true;
        }
    }

    return false;
}

// record 0026: a field with a value written on it is given that value by
// every constructor, the implicit one included
// a class always is, since building one writes its vptr
bool IrLowerer::constructs(IrTypeId type) {
    IrTypeTable* types = program->get_types();

    if (types->get_type(type)->kind == IR_ARRAY) {
        return constructs(types->get_type(type)->element);
    }

    if (types->get_type(type)->kind == IR_CLASS) {
        return true;
    }

    if (types->get_type(type)->kind != IR_STRUCT) {
        return false;
    }

    IrAggregate* aggregate = types->get_aggregate(type);
    Module* module = compilation->get_module(aggregate->module);
    AstQuery query;

    query.set_module(module);

    if (has_init(type)
        || (aggregate->base.index != 0 && constructs(aggregate->base))) {
        return true;
    }

    for (const IrField& field : aggregate->fields) {
        u32 node = module->get_symbols()->get_candidate(field.candidate)
                       ->ast_node;

        if (query.get_binding_expression(node) != 0 || constructs(field.type)) {
            return true;
        }
    }

    return false;
}

bool IrLowerer::declares_copy(IrTypeId type) {
    u8 kind = program->get_types()->get_type(type)->kind;

    return (kind == IR_STRUCT || kind == IR_CLASS)
        && (copy_init_of(type) != 0 || assigns_by_field(type));
}

bool IrLowerer::assigns_by_field(IrTypeId type) {
    u8 kind = program->get_types()->get_type(type)->kind;

    if ((kind != IR_STRUCT && kind != IR_CLASS) || copy_init_of(type) != 0) {
        return false;
    }

    IrAggregate* aggregate = program->get_types()->get_aggregate(type);

    if (aggregate->base.index != 0 && declares_copy(aggregate->base)) {
        return true;
    }

    for (const IrField& field : aggregate->fields) {
        if (declares_copy(field.type)) {
            return true;
        }
    }

    return false;
}

IrObjectId IrLowerer::vtable_of(IrTypeId type) {
    std::map<u32, u32>::iterator found = vtables.find(type.index);

    if (found != vtables.end()) {
        return {found->second};
    }

    const IrClass* entry = program->get_classes()->get_class(type);
    IrTypeTable* types = program->get_types();
    u32 each = target->pointer_size;
    u32 count = (u32) entry->slots.size() + 1;
    IrObjectId table = program->add_data(
        unique(types->get_aggregate(type)->name + ".vtable"),
        types->array(types->pointer(IR_SPACE_GENERIC), count),
        std::vector<u8>(count * each, 0), true);

    vtables[type.index] = table.index;
    program->add_relocation(table, 0, descriptor_of(type));

    for (u32 s = 0; s < entry->slots.size(); s++) {
        const IrSlot& slot = entry->slots[s];
        IrObjectId filled =
            s == 0 ? structor(STRUCTOR_DESTROY, type, 0)
                   : function_of(slot.module, slot.candidate);

        if (filled.index == 0) {
            fail("not lowered yet: the slot '" + slot.name + "' of '"
                 + types->get_aggregate(type)->name + "'");
            return table;
        }

        program->add_relocation(table, (s + 1) * each, filled);
    }

    return table;
}

// version 1 (record 0077, stage 1): the name, the size, the base's
IrObjectId IrLowerer::descriptor_of(IrTypeId type) {
    std::map<u32, u32>::iterator found = descriptors.find(type.index);

    if (found != descriptors.end()) {
        return {found->second};
    }

    IrTypeTable* types = program->get_types();
    IrAggregate* aggregate = types->get_aggregate(type);
    std::string name = aggregate->name;
    IrTypeId base = aggregate->base;
    u32 each = target->pointer_size;
    LayoutTable layouts;

    layouts.set_target(target);
    layouts.set_types(types);
    layouts.lay_out_all();

    u64 size = layouts.size_of(type);
    std::vector<u8> bytes(3 * each, 0);

    for (u32 i = 0; i < each; i++) {
        u32 at = target->big_endian ? each - 1 - i : i;

        bytes[each + at] = (u8) (size >> (8 * i));
    }

    IrObjectId made = program->add_data(
        unique(name + ".descriptor"),
        types->array(types->pointer(IR_SPACE_GENERIC), 3), bytes, true);

    descriptors[type.index] = made.index;
    program->add_relocation(made, 0, text(name));

    if (base.index != 0 && types->get_type(base)->kind == IR_CLASS) {
        program->add_relocation(made, 2 * each, descriptor_of(base));
    }

    return made;
}

i32 IrLowerer::slot_of_method(u32 module, u32 candidate) {
    u32 declaration = holder_of(module, candidate);

    if (declaration == 0
        || compilation->get_module(module)->get_ast()->get_node(declaration)
                   ->get_kind()
               != AST_CLASS) {
        return -1;
    }

    return program->get_classes()->slot_of(
        type_of_declaration(module, declaration), module, candidate);
}

IrTypeId IrLowerer::receiver_type(u32 module, u32 candidate) {
    IrTypeId holding = type_of_declaration(module, holder_of(module, candidate));
    i32 slot = slot_of_method(module, candidate);

    return slot < 0 ? holding : introducer_of(holding, (u32) slot);
}

IrTypeId IrLowerer::top_class(IrTypeId type) {
    return introducer_of(type, 0);
}

// a base's slots are the first of its derived class's, so the class that
// introduced a slot is the highest one whose vtable is that long
IrTypeId IrLowerer::introducer_of(IrTypeId type, u32 slot) {
    IrTypeTable* types = program->get_types();

    while (true) {
        IrTypeId base = types->get_aggregate(type)->base;

        if (base.index == 0 || types->get_type(base)->kind != IR_CLASS
            || program->get_classes()->get_class(base)->slots.size() <= slot) {
            return type;
        }

        type = base;
    }
}

// question 88: always, after the lowering and after flatten. A failure here
// is the compiler's bug, never the program's
bool IrLowerer::verify(const std::string& when) {
    IrVerifier verifier;

    verifier.set_program(program);

    if (verifier.verify()) {
        return true;
    }

    fail("the IR does not verify after " + when + ": "
         + verifier.get_errors()[0].message);

    return false;
}

bool IrLowerer::is_generic(u32 module_index, u32 declaration) {
    Module* module = compilation->get_module(module_index);
    AstQuery query;

    query.set_module(module);

    if (query.get_generic_parameters(declaration).size() == 0) {
        return false;
    }

    return module->get_instantiation(
               module->get_symbols()->candidate_of(declaration))
        == nullptr;
}
