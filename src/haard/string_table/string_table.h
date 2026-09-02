#ifndef HAARD_STRING_TABLE_H
#define HAARD_STRING_TABLE_H

#include <haard/haard.h>
#include <map>
#include <string_view>
#include <vector>

namespace haard {
    const u32 INVALID_STRING = 0xffffffff;

    // FNV-1a, 32 bits. Record 0014 pins it here, in the compiler's own source
    // and not in std::hash, because the number goes into the blob and is
    // compared against one computed by a later run, by another machine, and
    // eventually by the compiler rewritten in Haard. std::hash promises only
    // to be stable 'for the duration of the program', so a cache that outlived
    // its build would read every name as missing -- the bytes all present and
    // only the number disagreeing.
    //
    // It does not need to be a good hash. Every hit is confirmed by comparing
    // the bytes, so a collision costs one comparison and can never give a
    // wrong answer. The only property required is that it never changes
    u32 hash_name(std::string_view text);

    // one entry per distinct name of a module, per record 0013
    struct InternedString {
        u32 offset;
        u32 length;
        u32 hash;
    };

    // The names of one module, each stored once.
    //
    // Record 0012 makes every table per module and record 0013 has
    // Symbol::name index this one. Interning is what makes a name comparison
    // inside a module a u32 compare that never touches a byte.
    //
    // Two flat sections, a char pool and the entries, so loading a module is
    // resize() plus one memcpy each -- record 0004's promise. The map that
    // deduplicates while building is not one of them: it holds nothing the
    // entries do not already say, and a load rebuilds it or does without it
    class StringTable {
        public:
            StringTable();

        public:
            // the index of this text, appending it when it is new
            u32 intern(std::string_view text);

            // the index of a name that was hashed somewhere else: the lookup
            // that crosses a module boundary, where the importer's own index
            // means nothing here. The hash is the probe and the bytes confirm
            // it, and the confirm is not optional -- a u32 over a few thousand
            // names collides often enough to matter. INVALID_STRING when the
            // name is not in this module
            u32 find(u32 hash, std::string_view text);

            // good until the next intern, which may move the pool. Same rule
            // as Module::get_token_value, which views the source text
            std::string_view get_text(u32 index);
            u32 get_hash(u32 index);

            u32 get_count();

        private:
            std::vector<char> pool;
            std::vector<InternedString> strings;

            // hash -> the entries carrying it, so interning and find() are the
            // same probe against the same buckets
            std::map<u32, std::vector<u32>> by_hash;
    };
}

#endif
