#include <haard/ir/ir_type_table.h>

using namespace haard;

static const char* SCALAR_NAMES[] = {
    "none", "b8", "b16", "b32", "b64", "bool", "f32", "f64"
};

IrTypeTable::IrTypeTable() {
    IrTypeId nothing = {0};

    // index 0 is 'no type', and every scalar after it sits at its kind
    for (u32 kind = IR_TYPE_NONE; kind <= IR_F64; kind++) {
        add((IrTypeKind) kind, 0, nothing);
    }
}

IrTypeId IrTypeTable::scalar(IrTypeKind kind) {
    IrTypeId type = {(u32) kind};

    return type;
}

IrTypeId IrTypeTable::integer(u32 bytes) {
    switch (bytes) {
    case 1:
        return scalar(IR_B8);

    case 2:
        return scalar(IR_B16);

    case 4:
        return scalar(IR_B32);

    default:
        return scalar(IR_B64);
    }
}

IrTypeId IrTypeTable::pointer(u32 space) {
    std::map<u32, u32>::iterator found = pointers.find(space);

    if (found != pointers.end()) {
        IrTypeId type = {found->second};

        return type;
    }

    IrTypeId nothing = {0};
    IrTypeId made = add(IR_PTR, space, nothing);

    pointers[space] = made.index;

    return made;
}

IrTypeId IrTypeTable::array(IrTypeId element, u32 length) {
    std::pair<u32, u32> key(element.index, length);
    std::map<std::pair<u32, u32>, u32>::iterator found = arrays.find(key);

    if (found != arrays.end()) {
        IrTypeId type = {found->second};

        return type;
    }

    IrTypeId made = add(IR_ARRAY, length, element);

    arrays[key] = made.index;

    return made;
}

IrTypeId IrTypeTable::aggregate(IrTypeKind kind, const std::string& name) {
    IrAggregate made;
    IrTypeId nothing = {0};

    made.name = name;
    made.base = nothing;
    made.pod_for_layout = false;
    made.module = 0;
    made.declaration = 0;
    made.source_type = 0;
    aggregates.push_back(made);

    return add(kind, (u32) aggregates.size() - 1, nothing);
}

const IrType* IrTypeTable::get_type(IrTypeId type) {
    return &types[type.index];
}

IrAggregate* IrTypeTable::get_aggregate(IrTypeId type) {
    return &aggregates[types[type.index].subject];
}

bool IrTypeTable::is_aggregate(IrTypeId type) {
    u8 kind = types[type.index].kind;

    return kind == IR_STRUCT || kind == IR_CLASS || kind == IR_UNION;
}

u32 IrTypeTable::get_count() {
    return (u32) types.size();
}

std::string IrTypeTable::name_of(IrTypeId type) {
    const IrType* entry = get_type(type);

    switch ((IrTypeKind) entry->kind) {
    case IR_PTR:
        return entry->subject == IR_SPACE_GENERIC
                   ? "ptr"
                   : "ptr(" + std::to_string(entry->subject) + ")";

    case IR_ARRAY:
        return name_of(entry->element) + "[" + std::to_string(entry->subject)
             + "]";

    case IR_STRUCT:
    case IR_CLASS:
    case IR_UNION:
        return get_aggregate(type)->name;

    default:
        return SCALAR_NAMES[entry->kind];
    }
}

IrTypeId IrTypeTable::add(IrTypeKind kind, u32 subject, IrTypeId element) {
    IrType entry;
    IrTypeId made = {(u32) types.size()};

    entry.kind = kind;
    entry.subject = subject;
    entry.element = element;
    types.push_back(entry);

    return made;
}
