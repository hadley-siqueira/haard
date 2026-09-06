#include <haard/type_table/type_table.h>

using namespace haard;

TypeTable::TypeTable() {
    types.push_back(Type{TYPE_NONE, 0, 0, 0, 0});

    // seeded in the order of BuiltinType, so builtin(BUILTIN_I32) is the same
    // index in every module of every compilation
    for (u32 i = 0; i < BUILTIN_COUNT; i++) {
        intern(TYPE_BUILTIN, i, 0, std::vector<u32>());
    }
}

u32 TypeTable::builtin(BuiltinType which) {
    return which + 1;
}

u32 TypeTable::pointer(u32 type) {
    return intern(TYPE_POINTER, 0, 0, std::vector<u32>{type});
}

u32 TypeTable::reference(u32 type) {
    return intern(TYPE_REFERENCE, 0, 0, std::vector<u32>{type});
}

u32 TypeTable::list(u32 type) {
    return intern(TYPE_LIST, 0, 0, std::vector<u32>{type});
}

u32 TypeTable::array(u32 type, u32 length) {
    return intern(TYPE_ARRAY, length, 0, std::vector<u32>{type});
}

u32 TypeTable::hash(u32 key, u32 value) {
    return intern(TYPE_HASH, 0, 0, std::vector<u32>{key, value});
}

u32 TypeTable::tuple(const std::vector<u32>& elements) {
    return intern(TYPE_TUPLE, 0, 0, elements);
}

u32 TypeTable::function(const std::vector<u32>& parameters, u32 result) {
    std::vector<u32> arguments = parameters;

    arguments.push_back(result);

    return intern(TYPE_FUNCTION, 0, 0, arguments);
}

u32 TypeTable::named(u32 module, u32 candidate,
                     const std::vector<u32>& arguments) {
    return intern(TYPE_NAMED, candidate, module, arguments);
}

u32 TypeTable::generic(u32 module, u32 candidate) {
    return intern(TYPE_GENERIC, candidate, module, std::vector<u32>());
}

u32 TypeTable::intern(TypeKind kind, u32 subject, u32 module,
                      const std::vector<u32>& arguments) {
    // FNV-1a's mixing over the entry's fields. Unlike record 0014's hash this
    // one is **not** written down: it only groups candidates for 'same', which
    // compares the fields themselves. So a collision costs one comparison, and
    // this function may change without breaking anything on disk
    u32 hash = 0x811c9dc5;

    hash = (hash ^ kind) * 0x01000193;
    hash = (hash ^ subject) * 0x01000193;
    hash = (hash ^ module) * 0x01000193;

    for (u32 argument : arguments) {
        hash = (hash ^ argument) * 0x01000193;
    }

    for (u32 index : by_hash[hash]) {
        if (same(index, kind, subject, module, arguments)) {
            return index;
        }
    }

    Type entry;

    entry.kind = (u8) kind;
    entry.subject = subject;
    entry.module = module;
    entry.first_argument = (u32) pool.size();
    entry.argument_count = (u16) arguments.size();

    pool.insert(pool.end(), arguments.begin(), arguments.end());
    types.push_back(entry);
    by_hash[hash].push_back((u32) types.size() - 1);

    return (u32) types.size() - 1;
}

bool TypeTable::same(u32 index, TypeKind kind, u32 subject, u32 module,
                     const std::vector<u32>& arguments) {
    Type& entry = types[index];

    if (entry.kind != kind || entry.subject != subject
        || entry.module != module
        || entry.argument_count != arguments.size()) {
        return false;
    }

    for (u32 i = 0; i < arguments.size(); i++) {
        if (pool[entry.first_argument + i] != arguments[i]) {
            return false;
        }
    }

    return true;
}

u32 TypeTable::value_of(u32 index) {
    if (index == INVALID_TYPE) {
        return index;
    }

    Type* entry = get_type(index);

    // one level, and never a walk: 'T&&' does not parse, so a reference to a
    // reference is not a shape this table can hold
    if (entry->kind == TYPE_REFERENCE) {
        return get_argument(entry->first_argument);
    }

    return index;
}

Type* TypeTable::get_type(u32 index) {
    return &types[index];
}

u32 TypeTable::get_argument(u32 index) {
    return pool[index];
}

u32 TypeTable::get_count() {
    return (u32) types.size();
}

std::vector<u32> TypeTable::get_arguments(u32 index) {
    std::vector<u32> arguments;
    Type& entry = types[index];

    for (u32 i = 0; i < entry.argument_count; i++) {
        arguments.push_back(pool[entry.first_argument + i]);
    }

    return arguments;
}
