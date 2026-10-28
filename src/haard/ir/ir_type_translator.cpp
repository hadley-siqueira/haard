#include <haard/ir/ir_type_translator.h>

using namespace haard;

IrTypeTranslator::IrTypeTranslator() {
    compilation = nullptr;
    target = nullptr;
    types = nullptr;
    function_value = 0;
}

void IrTypeTranslator::set_compilation(Compilation* compilation) {
    this->compilation = compilation;

    typer.set_compilation(compilation);
}

void IrTypeTranslator::set_target(const Target* target) {
    this->target = target;
}

void IrTypeTranslator::set_types(IrTypeTable* types) {
    this->types = types;
}

bool IrTypeTranslator::translate_program() {
    for (u32 i = 0; i < compilation->get_module_count(); i++) {
        Module* module = compilation->get_module(i);
        AstQuery query;

        query.set_module(module);

        for (u32 declaration : query.get_declarations()) {
            AstNodeKind kind =
                module->get_ast()->get_node(declaration)->get_kind();

            if ((kind == AST_CLASS || kind == AST_STRUCT || kind == AST_ENUM
                 || kind == AST_UNION)
                && module->get_symbols()->candidate_of(declaration) != 0
                && !is_generic(i, declaration)) {
                of_declaration(i, declaration);
            }
        }
    }

    // the tuples no declaration holds, which are most of them: a value of
    // one in a local is a slot of that struct all the same
    for (u32 i = 0; i < compilation->get_module_count(); i++) {
        TypeTable* table = compilation->get_module(i)->get_types();

        for (u32 type = 1; type < table->get_count(); type++) {
            if (table->get_type(type)->kind == TYPE_TUPLE
                && is_concrete(i, type)) {
                of_tuple(i, type);
            }
        }
    }

    return error.size() == 0;
}

IrTypeId IrTypeTranslator::translate(u32 module, u32 type) {
    TypeTable* table = compilation->get_module(module)->get_types();
    Type* entry = table->get_type(type);

    switch ((TypeKind) entry->kind) {
    case TYPE_BUILTIN:
        switch ((BuiltinType) entry->subject) {
        case BUILTIN_U8:
        case BUILTIN_I8:
        case BUILTIN_CHAR:
            return types->scalar(IR_B8);

        case BUILTIN_U16:
        case BUILTIN_I16:
            return types->scalar(IR_B16);

        case BUILTIN_U32:
        case BUILTIN_I32:
            return types->scalar(IR_B32);

        case BUILTIN_U64:
        case BUILTIN_I64:
            return types->scalar(IR_B64);

        case BUILTIN_ISIZE:
        case BUILTIN_USIZE:
            return types->integer(target->pointer_size);

        case BUILTIN_F32:
            return types->scalar(IR_F32);

        case BUILTIN_F64:
            return types->scalar(IR_F64);

        case BUILTIN_BOOL:
            return types->scalar(IR_BOOL);

        // record 0041: a symbol is a pointer to its entry in a table
        case BUILTIN_SYMBOL:
            return types->pointer(IR_SPACE_GENERIC);

        default:
            return nothing();
        }

    case TYPE_POINTER:
    case TYPE_REFERENCE:
    case TYPE_MOVE_REFERENCE:
        return types->pointer(IR_SPACE_GENERIC);

    case TYPE_ARRAY: {
        if (entry->subject == NO_LENGTH) {
            return nothing();
        }

        IrTypeId element =
            translate(module, table->get_argument(entry->first_argument));

        return element.index == 0 ? nothing()
                                  : types->array(element, entry->subject);
    }

    case TYPE_TUPLE:
        return of_tuple(module, type);

    case TYPE_FUNCTION:
        return of_function(module, type);

    case TYPE_NAMED: {
        Module* owner = compilation->get_module(entry->module);

        return of_declaration(
            entry->module,
            owner->get_symbols()->get_candidate(entry->subject)->ast_node);
    }

    default:
        return nothing();
    }
}

const std::string& IrTypeTranslator::get_error() {
    return error;
}

IrTypeId IrTypeTranslator::translate_declaration(u32 module, u32 declaration) {
    return of_declaration(module, declaration);
}

IrTypeId IrTypeTranslator::of_declaration(u32 module_index, u32 declaration) {
    std::pair<u32, u32> key(module_index, declaration);
    std::map<std::pair<u32, u32>, u32>::iterator found =
        declarations.find(key);

    if (found != declarations.end()) {
        IrTypeId type = {found->second};

        return type;
    }

    Module* module = compilation->get_module(module_index);
    AstNodeKind kind = module->get_ast()->get_node(declaration)->get_kind();
    std::string name = name_of(module_index, declaration);

    if (kind == AST_ENUM) {
        return of_enum(module_index, declaration, name);
    }

    IrTypeId made = types->aggregate(kind == AST_CLASS   ? IR_CLASS
                                     : kind == AST_UNION ? IR_UNION
                                                         : IR_STRUCT,
                                     name);

    // written down before the fields are translated, so a field that points
    // back here finds it
    declarations[key] = made.index;

    Candidate* candidate = module->get_symbols()->get_candidate(
        module->get_symbols()->candidate_of(declaration));
    IrTypeId base = candidate->super == INVALID_TYPE
                        ? nothing()
                        : translate(module_index, candidate->super);
    std::vector<IrField> fields = fields_of(module_index, declaration, name);

    // asked for again: translating the fields may have added aggregates and
    // moved the one 'made' names
    IrAggregate* aggregate = types->get_aggregate(made);

    aggregate->base = base;
    aggregate->fields = fields;
    aggregate->pod_for_layout = is_pod_for_layout(module_index, declaration);
    aggregate->module = module_index;
    aggregate->declaration = declaration;

    return made;
}

IrTypeId IrTypeTranslator::of_enum(u32 module_index, u32 declaration,
                                   const std::string& name) {
    Module* module = compilation->get_module(module_index);
    SymbolTable* symbols = module->get_symbols();
    AstQuery query;

    query.set_module(module);

    u32 largest = query.get_largest_tag(declaration);
    IrTypeId tag = types->integer(largest <= 0xff     ? 1
                                  : largest <= 0xffff ? 2
                                                      : 4);
    std::vector<u32> variants = query.get_variants(declaration);
    std::vector<u32> carrying;

    for (u32 variant : variants) {
        if (module->get_ast()->get_node(variant)->get_kind() == AST_FIELD
            && query.get_written_type(variant) != 0) {
            carrying.push_back(variant);
        }
    }

    std::pair<u32, u32> key(module_index, declaration);

    // the tag is the whole value, unless a variant carries something or the
    // enum writes a method (a C++ 'enum class' can hold none)
    if (carrying.size() == 0
        && query.get_members(declaration).size() == variants.size()) {
        declarations[key] = tag.index;
        return tag;
    }

    IrTypeId made = types->aggregate(IR_STRUCT, name);
    std::vector<IrField> fields;
    IrField field;

    declarations[key] = made.index;

    field.name = "tag";
    field.type = tag;
    field.candidate = 0;
    fields.push_back(field);

    if (carrying.size() > 0) {
        IrTypeId payload = types->aggregate(IR_UNION, name + ".payload");
        std::vector<IrField> members;

        for (u32 variant : carrying) {
            u32 named = symbols->candidate_of(variant);
            std::string written = query.get_declaration_name(variant);
            IrTypeId carried = types->aggregate(IR_STRUCT,
                                                name + "." + written);
            std::vector<IrField> inside;

            // the signature of the variant's constructor, whose return --
            // the enum itself -- is last (record 0016)
            std::vector<u32> signature = module->get_types()->get_arguments(
                symbols->get_candidate(named)->type);

            for (u32 i = 0; i + 1 < signature.size(); i++) {
                field.name = "_" + std::to_string(i);
                field.type = translate(module_index, signature[i]);
                field.candidate = 0;
                inside.push_back(field);
            }

            IrAggregate* aggregate = types->get_aggregate(carried);

            aggregate->fields = inside;
            aggregate->pod_for_layout = true;
            aggregate->module = module_index;
            aggregate->declaration = variant;

            field.name = written;
            field.type = carried;
            field.candidate = named;
            members.push_back(field);
        }

        IrAggregate* aggregate = types->get_aggregate(payload);

        aggregate->fields = members;
        aggregate->pod_for_layout = true;
        aggregate->module = module_index;
        aggregate->declaration = declaration;

        field.name = "payload";
        field.type = payload;
        field.candidate = 0;
        fields.push_back(field);
    }

    IrAggregate* aggregate = types->get_aggregate(made);

    aggregate->fields = fields;
    aggregate->pod_for_layout = false;
    aggregate->module = module_index;
    aggregate->declaration = declaration;

    return made;
}

IrTypeId IrTypeTranslator::of_tuple(u32 module, u32 type) {
    TypeTable* table = compilation->get_module(module)->get_types();
    std::vector<u32> elements = table->get_arguments(type);
    std::vector<u32> key;
    std::vector<IrField> fields;
    std::string name;
    bool plain = true;

    for (u32 i = 0; i < elements.size(); i++) {
        IrField field;

        field.name = "e" + std::to_string(i);
        field.type = translate(module, elements[i]);
        field.candidate = 0;

        if (field.type.index == 0) {
            return nothing();
        }

        u8 kind = table->get_type(elements[i])->kind;

        if (kind == TYPE_REFERENCE || kind == TYPE_MOVE_REFERENCE) {
            plain = false;
        }

        key.push_back(field.type.index);
        fields.push_back(field);
        name += (i > 0 ? ", " : "") + types->name_of(field.type);
    }

    std::map<std::vector<u32>, u32>::iterator found = tuples.find(key);

    if (found != tuples.end()) {
        IrTypeId made = {found->second};

        return made;
    }

    IrTypeId made = types->aggregate(IR_STRUCT, "(" + name + ")");
    IrAggregate* aggregate = types->get_aggregate(made);

    aggregate->fields = fields;
    aggregate->pod_for_layout = plain;
    aggregate->module = module;
    aggregate->source_type = type;
    tuples[key] = made.index;

    return made;
}

IrTypeId IrTypeTranslator::of_function(u32 module, u32 type) {
    if (function_value != 0) {
        IrTypeId made = {function_value};

        return made;
    }

    IrTypeId made = types->aggregate(IR_STRUCT, "function");
    IrAggregate* aggregate = types->get_aggregate(made);
    IrField field;

    field.type = types->pointer(IR_SPACE_GENERIC);
    field.candidate = 0;
    field.name = "env";
    aggregate->fields.push_back(field);
    field.name = "call";
    aggregate->fields.push_back(field);
    aggregate->pod_for_layout = true;
    aggregate->module = module;
    aggregate->source_type = type;
    function_value = made.index;

    return made;
}

std::vector<IrField> IrTypeTranslator::fields_of(u32 module_index,
                                                 u32 declaration,
                                                 const std::string& owner) {
    Module* module = compilation->get_module(module_index);
    SymbolTable* symbols = module->get_symbols();
    std::vector<IrField> fields;
    AstQuery query;

    query.set_module(module);

    for (u32 member : query.get_members(declaration)) {
        if (module->get_ast()->get_node(member)->get_kind() != AST_FIELD) {
            continue;
        }

        IrField field;

        field.candidate = symbols->candidate_of(member);

        if (field.candidate == 0) {
            continue;
        }

        field.name = query.get_declaration_name(member);
        field.type = translate(module_index,
                               symbols->get_candidate(field.candidate)->type);

        if (field.type.index == 0 && error.size() == 0) {
            error = "the field '" + field.name + "' of '" + owner
                  + "' has no layout";
        }

        fields.push_back(field);
    }

    return fields;
}

// What makes the emitted C++ struct not POD for the purpose of layout, found
// by asking g++: a base; an 'init' (a C++ constructor) or a 'destroy' (a
// destructor); a field given a value (a member initialiser); a reference
// field. A method does not, and neither does a field whose own type is not
// POD
bool IrTypeTranslator::is_pod_for_layout(u32 module_index, u32 declaration) {
    Module* module = compilation->get_module(module_index);
    SymbolTable* symbols = module->get_symbols();
    AstQuery query;

    if (module->get_ast()->get_node(declaration)->get_kind() != AST_STRUCT
        || symbols->get_candidate(symbols->candidate_of(declaration))->super
               != INVALID_TYPE) {
        return false;
    }

    query.set_module(module);

    for (u32 member : query.get_members(declaration)) {
        AstNodeKind kind = module->get_ast()->get_node(member)->get_kind();

        if (kind == AST_FUNCTION) {
            std::string name = query.get_declaration_name(member);

            if (name == "init" || name == "destroy") {
                return false;
            }

            continue;
        }

        if (kind != AST_FIELD) {
            continue;
        }

        if (query.get_binding_expression(member) != 0) {
            return false;
        }

        u32 candidate = symbols->candidate_of(member);

        if (candidate == 0) {
            continue;
        }

        u8 type = module->get_types()
                      ->get_type(symbols->get_candidate(candidate)->type)
                      ->kind;

        if (type == TYPE_REFERENCE || type == TYPE_MOVE_REFERENCE) {
            return false;
        }
    }

    return true;
}

bool IrTypeTranslator::is_generic(u32 module_index, u32 declaration) {
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

bool IrTypeTranslator::is_concrete(u32 module, u32 type) {
    TypeTable* table = compilation->get_module(module)->get_types();

    if (table->get_type(type)->kind == TYPE_GENERIC) {
        return false;
    }

    for (u32 one : table->get_arguments(type)) {
        if (!is_concrete(module, one)) {
            return false;
        }
    }

    return true;
}

std::string IrTypeTranslator::name_of(u32 module_index, u32 declaration) {
    Module* module = compilation->get_module(module_index);
    AstQuery query;

    query.set_module(module);

    std::string name = query.get_declaration_name(declaration);
    const Instantiation* made = module->get_instantiation(
        module->get_symbols()->candidate_of(declaration));

    if (made != nullptr) {
        name += "<";

        for (u32 i = 0; i < made->arguments.size(); i++) {
            name += (i > 0 ? ", " : "")
                  + typer.name_in(module_index, made->arguments[i]);
        }

        name += ">";
    }

    return module->get_name().size() > 0 ? module->get_name() + "." + name
                                         : name;
}

IrTypeId IrTypeTranslator::nothing() {
    IrTypeId type = {0};

    return type;
}
