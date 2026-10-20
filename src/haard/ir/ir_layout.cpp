#include <haard/ir/ir_layout.h>

using namespace haard;

LayoutTable::LayoutTable() {
    target = nullptr;
    types = nullptr;
}

void LayoutTable::set_target(const Target* target) {
    this->target = target;
}

void LayoutTable::set_types(IrTypeTable* types) {
    this->types = types;
}

bool LayoutTable::lay_out_all() {
    for (u32 i = 1; i < types->get_count(); i++) {
        IrTypeId type = {i};

        if (types->is_aggregate(type) && !lay_out(type)) {
            return false;
        }
    }

    return true;
}

u32 LayoutTable::size_of(IrTypeId type) {
    const IrType* entry = types->get_type(type);

    switch ((IrTypeKind) entry->kind) {
    case IR_B8:
    case IR_BOOL:
        return 1;

    case IR_B16:
        return 2;

    case IR_B32:
    case IR_F32:
        return 4;

    case IR_B64:
    case IR_F64:
        return 8;

    case IR_PTR:
        return target->pointer_size;

    case IR_ARRAY:
        return size_of(entry->element) * entry->subject;

    case IR_STRUCT:
    case IR_CLASS:
    case IR_UNION:
        return layouts[entry->subject].size;

    default:
        return 0;
    }
}

u32 LayoutTable::align_of(IrTypeId type) {
    const IrType* entry = types->get_type(type);

    switch ((IrTypeKind) entry->kind) {
    case IR_ARRAY:
        return align_of(entry->element);

    case IR_STRUCT:
    case IR_CLASS:
    case IR_UNION:
        return layouts[entry->subject].align;

    // a scalar is aligned to its size, on every target there is
    default:
        return size_of(type);
    }
}

const IrLayout* LayoutTable::get_layout(IrTypeId type) {
    return &layouts[types->get_type(type)->subject];
}

const std::string& LayoutTable::get_error() {
    return error;
}

bool LayoutTable::lay_out(IrTypeId type) {
    u32 index = types->get_type(type)->subject;

    if (index >= states.size()) {
        states.resize(index + 1, 0);
        layouts.resize(index + 1);
    }

    if (states[index] == 2) {
        return true;
    }

    if (states[index] == 1) {
        error = "'" + types->name_of(type)
              + "' holds itself by value, so it has no size";
        return false;
    }

    states[index] = 1;

    IrAggregate* aggregate = types->get_aggregate(type);

    if (aggregate->base.index != 0 && !lay_out(aggregate->base)) {
        return false;
    }

    for (const IrField& field : aggregate->fields) {
        if (!lay_out_held(field.type)) {
            return false;
        }
    }

    // a fresh one, since 'layouts' may have moved while the fields were
    // laid out
    IrLayout layout;

    if (types->get_type(type)->kind == IR_UNION) {
        lay_out_union(type, layout);
    } else {
        lay_out_record(type, layout);
    }

    layouts[index] = layout;
    states[index] = 2;

    return true;
}

bool LayoutTable::lay_out_held(IrTypeId type) {
    const IrType* entry = types->get_type(type);

    if (entry->kind == IR_ARRAY) {
        return lay_out_held(entry->element);
    }

    return !types->is_aggregate(type) || lay_out(type);
}

// The Itanium allocation for a class with no virtual base: the vtable pointer
// or the class base that already has one at 0, then the base, then the fields,
// each at the first offset its alignment allows past what came before
void LayoutTable::lay_out_record(IrTypeId type, IrLayout& layout) {
    IrAggregate* aggregate = types->get_aggregate(type);
    IrTypeId base = aggregate->base;
    std::set<std::pair<u32, u32>> taken;
    u32 data = 0;
    u32 floor = 0;

    layout.align = 1;
    layout.base_offset = 0;
    layout.dynamic = types->get_type(type)->kind == IR_CLASS;
    layout.holds_an_empty = false;

    if (layout.dynamic) {
        if (base.index != 0 && get_layout(base)->dynamic) {
            // the primary base: its vtable pointer is this class's
            data = get_layout(base)->data_size;
            layout.align = get_layout(base)->align;
            base = IrTypeId{0};
            place(aggregate->base, 0, taken);
        } else {
            data = target->pointer_size;
            layout.align = target->pointer_size;
        }
    }

    if (base.index != 0) {
        const IrLayout* above = get_layout(base);

        if (above->empty) {
            // At 0, over the vtable pointer if there is one: the base is the
            // first subobject placed, so nothing of its type can be there
            // yet, and it adds no data -- only a floor under the size
            layout.base_offset = place(base, 0, taken);
            floor = layout.base_offset + above->size;
        } else {
            layout.base_offset = place(base, align_up(data, above->align),
                                       taken);
            data = layout.base_offset
                 + (types->get_aggregate(base)->pod_for_layout
                        ? above->size
                        : above->data_size);
        }

        if (above->align > layout.align) {
            layout.align = above->align;
        }
    }

    for (const IrField& field : aggregate->fields) {
        u32 alignment = align_of(field.type);
        u32 at = place(field.type, align_up(data, alignment), taken);

        layout.offsets.push_back(at);
        data = at + size_of(field.type);

        if (alignment > layout.align) {
            layout.align = alignment;
        }
    }

    layout.empty = !layout.dynamic && aggregate->fields.size() == 0
                && (aggregate->base.index == 0
                    || get_layout(aggregate->base)->empty);
    layout.data_size = data;
    layout.size = align_up(data > floor ? data : floor, layout.align);

    if (layout.size == 0) {
        layout.size = layout.align;
    }

    layout.holds_an_empty =
        layout.empty
        || (aggregate->base.index != 0 && holds_an_empty(aggregate->base));

    for (const IrField& field : aggregate->fields) {
        if (holds_an_empty(field.type)) {
            layout.holds_an_empty = true;
        }
    }
}

// C's: every field at 0, as big and as aligned as the largest
void LayoutTable::lay_out_union(IrTypeId type, IrLayout& layout) {
    IrAggregate* aggregate = types->get_aggregate(type);
    u32 largest = 0;

    layout.align = 1;
    layout.base_offset = 0;
    layout.dynamic = false;
    layout.empty = false;
    layout.holds_an_empty = false;

    for (const IrField& field : aggregate->fields) {
        layout.offsets.push_back(0);

        if (size_of(field.type) > largest) {
            largest = size_of(field.type);
        }

        if (align_of(field.type) > layout.align) {
            layout.align = align_of(field.type);
        }
    }

    layout.size = align_up(largest, layout.align);

    if (layout.size == 0) {
        layout.size = layout.align;
    }

    layout.data_size = layout.size;
}

u32 LayoutTable::place(IrTypeId type, u32 at,
                       std::set<std::pair<u32, u32>>& taken) {
    std::vector<std::pair<u32, u32>> inside;
    u32 step = align_of(type);
    bool clear = false;

    while (!clear) {
        inside.clear();
        empties_of(type, at, inside);
        clear = true;

        for (const std::pair<u32, u32>& one : inside) {
            if (taken.count(one) > 0) {
                clear = false;
            }
        }

        if (!clear) {
            at += step;
        }
    }

    taken.insert(inside.begin(), inside.end());

    return at;
}

void LayoutTable::empties_of(IrTypeId type, u32 at,
                             std::vector<std::pair<u32, u32>>& found) {
    const IrType* entry = types->get_type(type);

    if (!holds_an_empty(type)) {
        return;
    }

    if (entry->kind == IR_ARRAY) {
        u32 size = size_of(entry->element);

        for (u32 i = 0; i < entry->subject; i++) {
            empties_of(entry->element, at + i * size, found);
        }

        return;
    }

    const IrLayout* layout = get_layout(type);
    IrAggregate* aggregate = types->get_aggregate(type);

    if (layout->empty) {
        found.push_back(std::make_pair(entry->subject, at));
    }

    if (aggregate->base.index != 0) {
        empties_of(aggregate->base, at + layout->base_offset, found);
    }

    for (u32 i = 0; i < aggregate->fields.size(); i++) {
        empties_of(aggregate->fields[i].type, at + layout->offsets[i], found);
    }
}

bool LayoutTable::holds_an_empty(IrTypeId type) {
    const IrType* entry = types->get_type(type);

    if (entry->kind == IR_ARRAY) {
        return entry->subject > 0 && holds_an_empty(entry->element);
    }

    // a union's members are not looked inside: no rule about two empty
    // structs at one address reaches into one
    if (entry->kind != IR_STRUCT && entry->kind != IR_CLASS) {
        return false;
    }

    return get_layout(type)->holds_an_empty;
}

u32 LayoutTable::align_up(u32 value, u32 alignment) {
    return (value + alignment - 1) / alignment * alignment;
}
