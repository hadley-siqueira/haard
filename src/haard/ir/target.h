#ifndef HAARD_IR_TARGET_H
#define HAARD_IR_TARGET_H

#include <haard/haard.h>
#include <string>

namespace haard {
    // What the layout needs to know about a machine (record 0077, stage 1).
    //
    // Only that, for now: the memory regions, the timing table and the
    // registers belong to the back end (stage 5), and nothing above it reads
    // them. Every target of 2026-10-20 aligns a scalar to its own size, so
    // that is not a field either -- a target that does otherwise adds one.
    struct Target {
        std::string name;
        bool big_endian;

        // in bytes. A pointer is aligned to its size, and 'isize' and
        // 'usize' are this wide (record 0050)
        u32 pointer_size;
    };

    // Hadley, 2026-10-20: three targets from the start. 'x86_64-linux' and
    // 'arm64-linux' are little endian; 'pret' is his processor, big endian
    // and 64-bit, provisional until its ISA is settled. False when the name
    // is none of them
    bool find_target(const std::string& name, Target& target);
}

#endif
