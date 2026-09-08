# What a module costs, measured

Background note for agenda items 2.5, 3.1 and 3.4, measured on 2026-09-01 with
the real compiler. Everything here came out of one synthetic module of **9.999
lines / 220.636 bytes**, 206 classes and 824 `def` between functions and
methods, which parses clean. No generics in it — see the caveat at the end.

## Memory, and what a blob would weigh

| | `sizeof` | count | bytes | × source |
|---|---|---|---|---|
| `Token` | 8 | 46.891 | 375.128 | 1,70 |
| `AstNode` | 16 | 54.512 | 872.192 | 3,95 |
| both | | | **1.247.320** | **5,65** |

Both are POD with no pointers, so a blob of the token stream plus the ast is
exactly those bytes plus a header — the `memcpy` load record 0004 promised. Add
the source text (220.636) and it is 1,40 MiB, 6,65× the source.

Scanning takes **16,8 ms** and parsing **14,7 ms** at `-O2`. That 32 ms is what
any blob at all saves; carrying more of the module in the blob does not save
more of it.

## Where the ast goes

| kind | nodes | % |
|---|---|---|
| `AST_IDENTIFIER` | 15.144 | 27,8 |
| `AST_INTEGER_LITERAL` | 4.944 | 9,1 |
| `AST_BINDING_NAME` | 4.432 | 8,1 |
| `AST_BUILTIN_TYPE` | 2.990 | 5,5 |
| `AST_BLOCK` | 2.884 | 5,3 |

**Over a quarter of the ast is identifiers**, so name resolution will touch
15.000 nodes in a module this size.

**71% of the ast is inside function bodies** (38.728 of 54.512 nodes). An
importer never reads a body unless the declaration is generic, so a blob of
signatures only would be 252.544 bytes against 1.247.320 — five times smaller.
That number stops mattering the moment incremental compilation is a goal,
because incremental needs the bodies.

## Names

| | |
|---|---|
| identifier tokens | 15.148 |
| **distinct** identifiers | **638** |
| uses per distinct name | 23,7 |
| their characters | 6.073 bytes |
| an interned name table would be | 11.177 bytes — **0,90%** of tokens + ast |

## What a name comparison costs

4 million comparisons over 4.000 identifiers of 3 to 15 characters.
`std::string_view::operator==` compares **lengths first** and only reaches
`memcmp` for names of equal length, which is why the two string rows differ.

| | `-O2` | `-g`, the current build |
|---|---|---|
| `u32 == u32` (interned) | 1,96 ns | 12,8 ns |
| `string_view ==`, mixed lengths | 4,03 ns | 25,7 ns |
| `string_view ==`, all 8 chars | 8,28 ns | 45,3 ns |
| `std::hash<string_view>` of a name | 28,05 ns | 47,3 ns |
| hash map lookup, name text → `u32` | 81,53 ns | 268,9 ns |
| `std::string(string_view)` — what `text_of` does | 20,30 ns | 157,9 ns |

**The comparison is not where the cost is.** A memcmp is two to four times a
`u32` compare — nanoseconds — and both are noise beside the 28 ns of hashing a
name and the 81 ns of a hash map lookup, which **both designs pay** to get from
a token's text to any key at all. Interning does not avoid that step; it only
lets it be paid once per token instead of once per scope level.

At 15.148 identifiers and 81 ns, resolving every name in this module costs
**≈ 1,2 ms** against the 32 ms the front end already spends. In a thousand-line
file it is 0,12 ms.

## Four things worth fixing, found while measuring

1. **`AstNode` wastes 19% on padding.** It uses 13 bytes (`u8` + three `u32`)
   and occupies 16. Reordering does not help: with alignment 4, 13 rounds to 16
   either way. 163 KB in this module.
2. **The vectors hold more than they use.** Geometric growth left 65.536 slots
   in both: 524 KB held for 375 KB of tokens, 1.048 KB for 872 KB of ast —
   176 KB idle, 14% of the total. A `shrink_to_fit` at the end of the parse, or
   a `reserve` estimated from the source size, gives it back.
3. **`AstQuery::text_of` builds a `std::string` per call** — 20 ns and a heap
   allocation. It should return a `string_view` where the caller needs no
   ownership, and that matters more once name resolution is on the hot path.
4. **The project builds with `-std=c++20 -g` and no `-O2`.** The right-hand
   column above is today's reality: three to six times slower. Every timing
   argument is academic until there is a release build, and that is a line of
   `CMakeLists.txt`, not a design decision.

## The caveat

The module has **no generics**. Record 0002 instantiates generics in the
importer, so in a generic-heavy module the share of "body somebody will
actually read" rises and the 71% argument weakens. That measurement was offered
and not yet made.
