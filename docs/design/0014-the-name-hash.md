# 0014 — The name hash is FNV-1a, and it lives in the compiler's own source

Status: **decided**, 2026-09-02. A small record for a small function, written
because the function is small and the consequence of changing it is not.

## Context

Record 0013 made the name hash a **stored** value: it is written into the blob
next to the interned strings, and an importer probes a dependency's table by
comparing the hash it computed against the hash that was written. So the two are
produced at different moments, by different runs, and eventually by different
implementations of the compiler.

That rules out `std::hash`. The standard requires only that its value depend on
the argument *"for the duration of the program"* — not across runs, not across
implementations. libstdc++ and libc++ already differ for `std::string`, and
neither promises to stay put. A blob written before a toolchain change would
read as a table where every name is missing, with no crash and no diagnostic:
the bytes are all there, and only the number used to find them disagrees.

## Decision

**FNV-1a, 32-bit**, over the name's bytes exactly as they appear in the source —
no normalization, no case folding, no terminator.

```c
u32 hash = 0x811C9DC5;          // offset basis

for (u32 i = 0; i < length; ++i) {
    hash ^= (u8) bytes[i];
    hash *= 0x01000193;         // prime, truncating to 32 bits
}
```

The variant matters: this is FNV-1**a**, which xors before it multiplies. FNV-1
does the two in the other order and gives different numbers for the same input.

**And the function is written in the compiler's own source, not called out to
the standard library.** Hadley, 2026-09-02: *"é até bom que a função de hash
faça parte do código fonte do compilador ao invés de depender de função externa
da std."* The five lines above are the whole implementation and the whole
specification at the same time, which is what a second implementation needs.

## Why a weak hash is the right choice here

The name hash **does not need to be good**. Record 0013 confirms every hash hit
with a byte comparison, so a collision costs one extra `memcmp` and can never
cause a wrong answer. The only property required is **stability**: the same
bytes give the same number, forever, on every implementation.

That is the opposite of what agenda 3.4's `interface_hash` needs, where a
collision means *failing to rebuild something that changed* and quality is the
whole requirement. The two are different hashes with opposite demands, and this
record covers only the first. They must not become the same function by
accident.

## Consequences

- **Changing this function is a format version bump** (agenda 3.6). It is the
  only thing that makes an old blob be *rejected* rather than silently
  misread, and misreading is this decision's failure mode.
  [0015](0015-the-cache-is-best-effort.md) sharpens this: a `.hdm` is validated
  by the size and mtime of its `.hd`, and neither of those changes when the
  compiler does — so the version stamp is not a formality, it is the only thing
  between a toolchain upgrade and a cache read with the wrong hash.
- **The Haard rewrite transcribes five lines.** That was the point of writing it
  down: a comment in the C++ source would not survive the rewrite, and the two
  implementations have to agree byte for byte or neither can read the other's
  cache.
- **It adds no dependency**, which is a standing rule of the project and here
  costs nothing to keep.
- It belongs next to the interned string table of record 0013, since that is the
  only thing that calls it.

## Rejected

**`std::hash`.** Convenient, and unusable for a value that outlives the process
that computed it, for the reason in the context above.

**A stronger hash** — anything in the xxHash or Murmur family. They are better
hashes and the extra quality buys nothing: the byte compare already makes
collisions free of consequence, and every one of them is more code to transcribe
into Haard than five lines.
