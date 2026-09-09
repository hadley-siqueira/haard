# Can `hdc` be written in Haard yet?

Written 2026-09-08, at Hadley's asking: *"in the compiler's current state
(haard → transpile → c++ → binary), would it already be possible to implement
the Haard compiler in Haard itself? Which structures still need to be
implemented?"*

Everything below was **measured against `src/`**, and every claim about what
Haard does or does not do was **run through `hdc`** rather than remembered.

## The answer

**Not yet — and the gap is five things, not fifty.** What is missing is
language features and a handful of natives, not architecture: nothing about the
compiler's *shape* is out of reach, and the parts of Haard that took the
longest to build are exactly the parts it needs.

## What the compiler is made of

20,550 lines, 16,030 of them `.cpp`, in 23 modules. The largest are
`parser.cpp` (2,566), `emitter.cpp` (2,331) and `expression_typer.cpp` (1,625).

And it is written in the subset Hadley imposed for exactly this: **no
templates, no lambdas, no inheritance, no virtual, two `runtime_error`s**. Its
24 data structures are plain structs of integers, held in vectors and threaded
by index — which is the one design decision that makes this whole question
cheap, because a `u32` index costs the same in both languages.

| what `src/` uses | how often | what Haard has |
|---|---|---|
| `std::string` | 355 | `String` ✅ |
| `std::vector` | 227 | `Array<T>` ✅ |
| `std::filesystem` | 62 | **nothing** ❌ |
| `std::to_string` | 24 | `append(i64)` ✅ |
| `std::string_view` | 16 | `char*` ✅ |
| `std::ostream` / `cout` / `cerr` / file streams | 31 | `File`, `console()` ✅ |
| `std::map` / `set` / `unordered_map` | 20 | `Hash<K, V>` ✅ (2026-09-08) |
| `std::pair` | 5 | a struct of two fields ✅ |
| `sort`, `stable_sort`, `upper_bound`, `swap`, `distance` | 6 | **library work** |
| `switch` | 36 | `if` / `elif` chains |
| the ternary `? :` | 133 | `if` / `else` statements |
| range-`for` | 58 | `for x in` ✅ |
| `static_cast` and friends | 6 | `as` ✅ |
| `sizeof` | 16 | `sizeof` ✅ |
| exceptions | 2 | return codes |

## The five blockers

### 1. Enums that reach the emitter — by far the largest

**1,934 references** to enum values across `src/`: `TK_*`, `AST_*`, `TYPE_*`,
`SYMBOL_*`, `BUILTIN_*`, `OVERLOAD_*`. Eight enum declarations, two of them
enormous — `TokenKind` has 115 members and `AstNodeKind` has 135.

Today an `enum` declares and the declaration emits, but a **member access does
not type**:

```haard
let k = TokenKind.TK_IDENTIFIER     # hdc: 'k' has no type the emitter can write
```

It passes the type phase in silence and dies in the emitter, which is the worst
of the two places for it to fail. Until a member has a type, a comparison, and
an emission, the scanner cannot be written at all — it is 115 constants and a
`switch` over them.

Nothing about it needs a decision that has not already been made elsewhere: an
enum is a type whose members are named values of it. Whether they are C++'s
`enum class`, a plain `enum`, or i32 constants is the emitter's business.

### 2. `main` with arguments

`Emitter::emit_main` looks for *"a `main` that takes nothing"* and refuses
anything else. There is no `argc`, no `argv`, and so no command line — a
compiler that cannot be told what to compile is not one.

Small: one signature the emitter recognises, and the C++ `main` forwarding to
it.

### 3. The file system

`std::filesystem` is used **62 times**, and the operations are:
`path` (joining, `parent_path`, `filename`, `extension`, `string`),
`weakly_canonical`, `relative`, `is_regular_file`, `is_directory`, `file_size`
and one `directory_iterator`.

Record 0030's eight natives cover open, read, write, close, end-of-file and the
three standard handles. What is missing is roughly six more natives — exists,
is a directory, size, list a directory, canonicalise, current directory — and a
`Path` class in the library above them. `ModuleFinder` is the one phase that
cannot be written without it.

Worth noticing: record 0010 already keeps the compiler from *searching* for
anything, so what is needed is the small end of a file system and not the
large one.

### 4. `&xs[i]` — **fixed on 2026-09-08**

It was the idiom every table in this compiler is written with —
`return &types[index]`, a method handing out a pointer into its own storage —
and over an `Array<T>` whose `operator[]` gives back a `T&` it did not type:
*expected `Entry*`, found `Entry&*`*.

Record 0035 says a reference **is** the thing it names, and `address_of` was
the one place that had not been told. One line. Fixed, with the case in
`tests/emitter/cases/a_reference_is_the_thing_it_names`.

### 5. Every method of a generic is instantiated — **needs a decision**

```haard
class Entry:            # a plain struct, like Token or AstNode
    kind : u8
    value : i32

let items : Array<Entry>
```

fails with *`Entry` has no `operator==`* — from `Array<T>::equals`, which
nothing called. Record 0002 clones the **whole** declaration, so every method
of the clone is typed, while C++ instantiates only what is used.

The compiler holds vectors of six such structs — `Token`, `AstNode`, `Type`,
`Symbol`, `Candidate`, `Scope` — and none of them would declare `==`. Three
ways out:

1. **Write `==` on each struct.** Six times five lines, and it costs nothing at
   run time. It is also six pieces of code nobody asked for.
2. **Instantiate a method when it is used**, which is C++'s rule and a real
   amendment to record 0002. It is the answer that scales, and it is the one
   that makes a container library possible at all — every `Array<T>` method
   that touches `T` is otherwise a requirement on `T`.
3. **Move `equals` out of `Array<T>`** into a free function, which fixes this
   case and none of the next ones.

This is the only one of the five that is Hadley's to decide.

## Not blockers, but volume

**36 `switch` statements**, some over 40 cases, become `if` / `elif` chains.
**133 ternaries** become a temporary and an `if`, which is invasive in code
that is mostly expressions. Neither is a decision the language has taken:
`switch` and `? :` simply are not in the grammar. If they are wanted, wanting
them **before** translating 36 and 133 of them is the cheap order.

**The library** needs: `String.substr`, `String.find`, an ordering for sorting,
a `sort` and a binary search (3 uses), `swap`, and reading a whole file at once.
All of it is Haard, none of it is compiler work.

## What is already there, and it is the hard part

Generics that monomorphise, so `Array<T>`, `List<T>` and `Hash<K, V>` cover
`vector`, `map` and `set`. `T&` parameters, which is what the compiler's
`u32& owner` out-parameters are. Pointers, `new T[n]`, `delete[]`, `null`.
Overloading with default arguments, which the compiler leans on heavily.
Modules with mutual imports and a prelude, which is exactly its 23-module
shape. `String` with template strings, which is what every diagnostic is made
of. And `for x in`, which is 58 range-`for` loops.

## How to do it, when it is time

**Do not translate all 20,000 lines at once.** Start with the **scanner**: it
needs enums, `String`, `Array` and `File`, and nothing else on this page. Its
suite is 45 golden files of text, and a scanner written in Haard can be checked
against **the same goldens** the C++ one passes — same input, same output, no
new test machinery. If it goes golden for golden, everything after it is volume
rather than risk.

Then the parser against `tests/parser`'s 398, and so on outward. Each phase has
a suite that compares text, which is the property that makes a bootstrap
measurable instead of hopeful.

## The order

1. **Enums** — unlocks 1,934 uses and the scanner
2. ~~`&xs[i]`~~ — done 2026-09-08
3. **Method instantiation on use** (or `==` on six structs) — Hadley's call
4. **`main(argc, argv)`** and the file-system natives
5. **The library**: `substr`, `find`, `sort`, `Path`
6. Optional and cheaper first: **`switch`** and the **ternary**
