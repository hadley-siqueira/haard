# Should a `char` have a sign?

Written 2026-11-04 for Hadley to decide. **Decided the same day: no sign**,
record [0081](../0081-a-char-has-no-sign.md). The note is kept as the comparison
he read, and two of its lines turned out wrong: `c == 0xC3` is refused, with or
without a sign (a whole number is not a `char`), and the emitter cannot simply
write the unsigned type, which the record explains. `c == '\xC3'` is how it is
written since record [0082](../0082-a-byte-is-written-by-two-digits.md).

## What was measured

`let c : char = 'z' + 10`, whose bits are `0x84`, on x86-64:

| Expression | IR | C++ path | C++ with `-funsigned-char` |
|---|---|---|---|
| `(c as i32) == 132` | yes | **no** | yes |
| `c > 'a'` | yes | **no** | yes |
| `c - 'a' > 0` | yes | **no** | yes |
| `(c as i64) > 0` | yes | **no** | yes |
| `(c as u8) == 132` | yes | yes | yes |

So the two paths disagree today on almost everything past 127. Two more effects:

- **`Hash`**: `std/hash.hd` does `h ^ (key[i] as u32)`, so a key with a character
  past 127 hashes differently on the two paths, and the order of walking the
  `Hash` may differ. This was read from the source, not run.
- **ARM**: C++'s `char` is usually unsigned on ARM Linux, so the same Haard
  program would change its result between x86-64 and ARM, both targets of
  Haard. This comes from what is known of the platform, not from a run.

## The comparison

| Criterion | Signed | Unsigned |
|---|---|---|
| Range | -128 to 127 | 0 to 255 |
| Order of text | bytes past 127 come before `'a'` | the order of UTF-8's code points, and of `strcmp` |
| Used as a table's index | a negative value leaves the table (C's `isalpha(c)` bug) | always 0 to 255 |
| Testing for ASCII | `c >= 0` | `c < 128` |
| Writing `c == 0xC3` | does not match without a conversion | works as written |
| An end-of-file sentinel (`-1`) | fits in the type | does not fit |
| Cost in hardware | a load that extends the sign | a load that extends with zeroes |
| The C++ path today, on x86-64 | already so | has to change |
| The IR today | has to change | already so |

Little is left in favour of a sign. The sentinel is not needed, since
`std.low_io` already has `__io_at_end`. Testing for ASCII costs the same either
way, and so, in practice, does the load on x86 and ARM. The one real advantage
is leaving the C++ path as it is.

Against having no sign: someone subtracting characters may expect a negative,
which record 0079 already answers, since `char - char` is an `i32`. `c - 1` with
`c` as `'\0'` wraps to 255, which is the eight-bit rule already chosen there.

## Other languages

From what is known of each; none was checked for this note.

| Language | `char` or its equivalent | Sign | Note |
|---|---|---|---|
| C and C++ | `char`, 8 bits | depends on the platform | signed on x86, usually unsigned on ARM Linux; the case to avoid |
| C++20 | `char8_t` | none | made for UTF-8 |
| D | `char`, 8 bits, a UTF-8 unit | none | `wchar` and `dchar` for the other widths |
| Go | `byte` (`uint8`) and `rune` (`int32`) | byte none, rune signed | indexing a string gives a `byte` |
| Rust | `char`, 32 bits, a Unicode value; `u8` for bytes | none | `char` has no arithmetic, only `as` |
| Zig | none; `u8` | none | `'a'` is a compile-time integer |
| Java | `char`, 16 bits | none | its `byte` is signed, hence `b & 0xFF` |
| C# | `char`, 16 bits | none | `byte` has no sign either |
| Swift | `Character`; `UInt8` for bytes | none | `CChar` varies by platform, for C only |
| Python | none; `bytes` gives integers | none | 0 to 255 |

The closest to Haard is D: 8 bits, no sign, distinct from `u8`. That fits record
0079's `char ± whole`, which Rust refuses.

## What no sign would change

- **IR**: nothing.
- **C++ path**: it has to treat `char` as unsigned. `-funsigned-char` gave the
  right answers above, but it is fragile, since each project's `Makefile` calls
  g++ with its own flags. Safer is the emitter writing it in the C++ it
  generates; what that costs around string literals is not measured yet.
- **Records**: a new record fixes the rule, and the pending item comes off.
- **Whoever wants the signed value**: writes `c as i8`.
