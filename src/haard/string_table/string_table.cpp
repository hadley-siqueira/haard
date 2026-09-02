#include <haard/string_table/string_table.h>

using namespace haard;

u32 haard::hash_name(std::string_view text) {
    u32 hash = 0x811c9dc5;

    for (size_t i = 0; i < text.size(); i++) {
        hash ^= (u8) text[i];
        hash *= 0x01000193;
    }

    return hash;
}

StringTable::StringTable() {
}

u32 StringTable::intern(std::string_view text) {
    u32 hash = hash_name(text);
    u32 found = find(hash, text);

    if (found != INVALID_STRING) {
        return found;
    }

    InternedString entry;

    entry.offset = (u32) pool.size();
    entry.length = (u32) text.size();
    entry.hash = hash;

    pool.insert(pool.end(), text.begin(), text.end());
    strings.push_back(entry);
    by_hash[hash].push_back((u32) strings.size() - 1);

    return (u32) strings.size() - 1;
}

u32 StringTable::find(u32 hash, std::string_view text) {
    auto bucket = by_hash.find(hash);

    if (bucket == by_hash.end()) {
        return INVALID_STRING;
    }

    // the bytes decide, not the hash. Two names of one module may share a
    // hash, and then the bucket holds both
    for (u32 index : bucket->second) {
        if (get_text(index) == text) {
            return index;
        }
    }

    return INVALID_STRING;
}

std::string_view StringTable::get_text(u32 index) {
    InternedString& entry = strings[index];

    return std::string_view(pool.data() + entry.offset, entry.length);
}

u32 StringTable::get_hash(u32 index) {
    return strings[index].hash;
}

u32 StringTable::get_count() {
    return (u32) strings.size();
}
