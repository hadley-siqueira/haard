#include <haard/ir/ir_class_table.h>

using namespace haard;

IrClassTable::IrClassTable() {
    compilation = nullptr;
    types = nullptr;
}

void IrClassTable::set_compilation(Compilation* compilation) {
    this->compilation = compilation;
}

void IrClassTable::set_types(IrTypeTable* types) {
    this->types = types;
}

void IrClassTable::build() {
    for (u32 i = 1; i < types->get_count(); i++) {
        IrTypeId type = {i};

        if (types->get_type(type)->kind == IR_CLASS) {
            build_class(type);
        }
    }
}

const IrClass* IrClassTable::get_class(IrTypeId type) {
    std::map<u32, u32>::iterator found = by_type.find(type.index);

    return found == by_type.end() ? nullptr : &classes[found->second];
}

i32 IrClassTable::slot_of(IrTypeId type, u32 module, u32 candidate) {
    const IrClass* entry = get_class(type);

    if (entry == nullptr) {
        return -1;
    }

    for (u32 i = 0; i < entry->slots.size(); i++) {
        if (entry->slots[i].module == module
            && entry->slots[i].candidate == candidate) {
            return (i32) i;
        }
    }

    return -1;
}

// the base first, so its slots are there to be taken over
void IrClassTable::build_class(IrTypeId type) {
    if (by_type.count(type.index) > 0) {
        return;
    }

    IrAggregate* aggregate = types->get_aggregate(type);
    IrTypeId base = aggregate->base;
    IrClass made;

    made.type = type;

    if (base.index != 0 && types->get_type(base)->kind == IR_CLASS) {
        build_class(base);
        made.slots = get_class(base)->slots;
    } else {
        IrSlot destructor;

        destructor.name = "destroy";
        destructor.module = 0;
        destructor.candidate = 0;
        destructor.implemented_by = type;
        made.slots.push_back(destructor);
    }

    u32 module_index = aggregate->module;
    Module* module = compilation->get_module(module_index);
    SymbolTable* symbols = module->get_symbols();
    AstQuery query;

    query.set_module(module);

    for (u32 member : query.get_members(aggregate->declaration)) {
        if (module->get_ast()->get_node(member)->get_kind() != AST_FUNCTION
            || is_generic(module_index, member)) {
            continue;
        }

        std::string name = query.get_declaration_name(member);
        u32 candidate = symbols->candidate_of(member);

        if (name == "init" || candidate == 0) {
            continue;
        }

        IrSlot slot;

        slot.name = name;
        slot.module = module_index;
        slot.candidate = candidate;
        slot.implemented_by = type;

        if (name == "destroy") {
            made.slots[0] = slot;
            continue;
        }

        bool overrides = false;

        for (u32 i = 1; i < made.slots.size() && !overrides; i++) {
            if (made.slots[i].name == name
                && same_parameters(made.slots[i].module,
                                   made.slots[i].candidate, module_index,
                                   candidate)) {
                made.slots[i] = slot;
                overrides = true;
            }
        }

        if (!overrides) {
            made.slots.push_back(slot);
        }
    }

    by_type[type.index] = (u32) classes.size();
    classes.push_back(made);
}

bool IrClassTable::same_parameters(u32 module, u32 candidate,
                                   u32 other_module, u32 other) {
    TypeTable* mine = compilation->get_module(module)->get_types();
    TypeTable* theirs = compilation->get_module(other_module)->get_types();
    std::vector<u32> first = mine->get_arguments(
        compilation->get_module(module)->get_symbols()->get_candidate(
            candidate)->type);
    std::vector<u32> second = theirs->get_arguments(
        compilation->get_module(other_module)->get_symbols()->get_candidate(
            other)->type);

    if (first.size() != second.size()) {
        return false;
    }

    // the return is last (record 0016), and not compared
    for (u32 i = 0; i + 1 < first.size(); i++) {
        if (!same_type(module, first[i], other_module, second[i])) {
            return false;
        }
    }

    return true;
}

bool IrClassTable::same_type(u32 module, u32 type, u32 other_module,
                             u32 other) {
    TypeTable* mine = compilation->get_module(module)->get_types();
    TypeTable* theirs = compilation->get_module(other_module)->get_types();
    Type* first = mine->get_type(type);
    Type* second = theirs->get_type(other);

    if (first->kind != second->kind || first->subject != second->subject
        || first->argument_count != second->argument_count) {
        return false;
    }

    if ((first->kind == TYPE_NAMED || first->kind == TYPE_GENERIC)
        && first->module != second->module) {
        return false;
    }

    std::vector<u32> inside = mine->get_arguments(type);
    std::vector<u32> outside = theirs->get_arguments(other);

    for (u32 i = 0; i < inside.size(); i++) {
        if (!same_type(module, inside[i], other_module, outside[i])) {
            return false;
        }
    }

    return true;
}

bool IrClassTable::is_generic(u32 module_index, u32 declaration) {
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
