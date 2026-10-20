# 0077 — An intermediate representation, for the processor and beyond

Status: **decided** for the IR itself, 2026-10-20 (first decisions
2026-10-08). Nothing is built. The answers are under each question in
[notes/ir-questions.md](notes/ir-questions.md), explained in
[notes/ir-questions-explained.md](notes/ir-questions-explained.md); the whole
discussion, every proposal and why it was replaced, is
[notes/designing-the-ir.md](notes/designing-the-ir.md). Questions 1-24
(language and processor) and 78-91 (the IR's own design, block G) are
answered; 25-77 (the processor, memory, SIMD and the CGRA, real time,
targets) stay open with their recommendations, and touch only the processor's
back end.

| | |
|---|---|
| An IR of our own, with no dependency (LLVM, MLIR and QBE are models, not libraries) | **decided**, Hadley |
| The C++ back end **stays** beside it | **decided**, Hadley (record 0025's portability) |
| Source variables in **memory form** first; SSA (`mem2reg`) later, as a pass | **decided**, Hadley |
| An **IR interpreter** is the test oracle: the same exit status as the C++ path | **decided**, Hadley |
| The IR's text is a **dump for reading** and goldens; nothing parses it back | **decided**, Hadley |
| Targets: his **PRET** processor (64-bit, big endian, floating point, scratchpads, dedicated registers, ISA in design), plus native **x86-64 and ARM** | **stated**, Hadley |
| Goals the IR must serve: **WCET** and real-time analyses, **scratchpad placement and overlays**, **SIMD**, a **CGRA** | **stated**, Hadley |
| Aim for the best design, even over his earlier choices; the IR **as high level as possible** | **instruction**, Hadley |
| Upper bits of a 32-bit result undefined | decided by Hadley for an untyped IR, **moot** now: a typed IR has no such bits, and each back end answers it for its ISA |
| One IR with a structured and a CFG form, typed, fields by name, objects by symbol, placement as a table, plain data by index | **decided**, Hadley, questions 78–91 (below) |
| `goto` and `label` removed from Haard, so every function is structured | **decided and built**, record [0078](0078-goto-and-label-are-removed.md) |
| Reserved for open questions: an address space in the pointer type (35), a bound on every loop (58), a barrier class of effects (56, 64) | **decided**, Hadley, 2026-10-20 |

## Context

Record 0025 chose no IR for the C++ back end and named the signals that
would make that wrong. Two have arrived: targets with real distance (the
processor, x86-64, ARM) and the work C++ did in silence (destructors and
temporaries at computed points, copies, vtables, layout of every aggregate).
On top of them came goals no transpiler serves: a WCET for a precision-timed
machine, deciding what lives in which scratchpad, and keeping loops visible
enough to vectorise them and map them onto a CGRA.

## What was decided, and what was replaced

Four shapes were proposed in two days (the notes keep each one and its
reasons): LLVM's typed CFG; QBE's level, with untyped 64-bit temporaries and
the width in the opcode; two IRs, a structured one above a QBE-level one; and
the current proposal, **one typed IR with two control forms**. Each step was
driven by a new requirement:

- **Several native targets** did not move it: a low IR serves amd64, arm64
  and riscv64 (QBE does).
- **SIMD and the CGRA** did: they need loops, induction variables, strides and
  aliasing facts, which a CFG of byte offsets has to recover by heavy
  analysis. MLIR exists for this reason.
- **Hadley's instruction to stay high level** removed the untyped level: once
  tables of structs, classes and layout exist, types cost little and give the
  verifier its rules.
- **WCET** made the structured form central (a loop region carries its
  bound), made whole-program compilation necessary and added flow facts,
  timing barriers and a machine ↔ IR ↔ source mapping.
- **Scratchpads** made every object in memory a named entity, referred to by
  symbol, with placement as a table applied at the end, so placing and
  analysing can iterate.

## The IR, as decided (block G, 2026-10-20)

- **Control (78).** One IR, two forms: **structured** (regions `if`, `loop`,
  `for`, `block`, with `break` and `continue`), which the lowering produces
  and loop passes work on, and **CFG**, below it. A function is in one form
  at a time; flatten is one way and copies each loop's bound onto its
  header; the interpreter runs only the CFG form.
- **Types (79).** Integers carry only a width: `b8`, `b16`, `b32`, `b64`;
  with `bool`, `f32`, `f64` and pointers. Signedness is in the opcode, in
  pairs (`sdiv`/`udiv`, `srem`/`urem`, `sshr`/`ushr`, signed and unsigned
  compares, `sext`/`zext`, `sitofp`/`uitofp` and back, `smulhi`/`umulhi`).
  `char` is `b8`; `isize`/`usize` take the target's width.
- **Pointers and aggregates (80).** An opaque `ptr` with an address space
  (generic by default), the type written on the instruction (`load b32,
  %p`, `field %p, Point.x`, `index %p, %i : f32`). Aggregates are never
  values, always slots: a by-value argument is a copy the caller passes by
  pointer, a by-value result is built into a destination pointer. Address
  instructions `field`, `index`, `upcast`, `downcast`. Tables for struct,
  class (base, fields, vtable slots, destructor), union and fixed array;
  an enum is a `{tag; union}` struct, a tuple a struct per shape. Layout in
  declaration order, natural alignment, the vptr first, computed per
  target; `sizeof` is a `size T` constant.
- **Objects and placement (81).** Every object in memory is named:
  functions, globals, literals, vtables, constant pools and jump tables,
  one stack per hardware thread; a heap is a region. Objects carry their
  initial bytes, or are zero-filled, and may be read-only. A separate
  table maps each whole object to a region or an overlay group; regions
  belong to the target description, the table to the program.
- **Locals (82).** Every local is a slot, parameters included (copied in
  at entry); values are only instruction results; mem2reg later.
- **Classes and cleanups (83).** `vcall Class.method` stays until
  instruction selection; a by-value receiver and `super.method()` call
  directly. The vptr follows the C++ rule during construction and
  destruction. The lowering emits every destructor call through a cleanup
  stack, innermost first, at every exit; the return value is written
  first; temporaries per record 0068, a conditional one guarded by a flag
  slot; a moved-from object is still destroyed; `new`/`delete` go through
  `__alloc`/`__free`, `delete` of null doing nothing; a by-value argument
  is copied and destroyed by the caller, a result built in its
  destination.
- **Loop bounds (84).** A constant maximum per entry, a mandatory field of
  the loop region (and of the header after flatten); only a pass that
  rewrites a loop's bound may change the loop; the interpreter checks every
  bound on every run.
- **Barriers (85).** Timing instructions, volatile accesses and memory
  transfers: nothing crosses them, they are never removed, duplicated in
  execution or speculated, and the WCET sees them. Every opcode declares its
  effects in one table every pass asks. A call is a full barrier in the
  first version.
- **Source positions (86).** `(module, byte offset)` per instruction, in a
  side table, required by the builder; compiler-made instructions take the
  position of the construct responsible; a merge keeps one position; an
  `inlined_at` reserved; machine instructions keep the id of their IR
  instruction; each slot keeps its declaration for a debugger.
- **The interpreter (87).** Memory by object (Miri, CompCert): a pointer is
  `(object, offset)`, with a fictitious base address for integer
  conversions; it stops on an access outside an object, a use after free,
  a double free and a non-resident overlay. MMIO through device models.
  Garbage upper bits, garbage new memory, loop bounds and (on the
  processor) cycles ≤ WCET are checked by default in the tests.
- **The verifier (88).** Always on, production included, with a flag to
  turn it off; it checks form, types, dominance, calls, objects and side
  tables; a failure is an internal error naming the pass.
- **Whole program (89).** One IR per program, only what is reachable, C
  functions opaque, the front end the only thing a cache may keep.
- **WCET (90).** Computed by hdc, tree-based (timing schema) first, sound
  because every function is structured; IPET with a solver of our own
  later. Per-function WCET, worst path in source lines, worst stack use;
  nothing on x86/ARM.
- **Storage (91).** Plain data by id per function: typed 4-byte ids,
  fixed-size instructions with an operand pool, index-linked order inside
  a block, regions as instructions owning blocks, program tables global,
  side tables as parallel vectors, compaction between passes with one
  function that knows every side table, no use lists (an alias table),
  access always by id.

## Stage 1: four details of the layout (Hadley, 2026-10-20)

- An **empty struct** has size 1, as in C++: two objects never share an
  address, and the C++ path agrees.
- An **enum's tag** is the smallest unsigned width that holds its variant
  count (`b8` up to 256 variants).
- A **`bool`** in memory is 1 byte.
- Stage 1 starts with **three targets**: `x86_64-linux` and `arm64-linux`
  (little endian, 64-bit) and `pret` (big endian, 64-bit, provisional until
  the ISA is settled).
- **Inheritance follows the Itanium C++ ABI**, as g++ lays the emitted C++
  out on x86-64 and ARM64: a base that is not POD for the purpose of layout
  lends its tail padding to the derived class's first fields, and an empty
  base takes no bytes (the empty base optimisation). Chosen over a simpler
  rule of our own so that g++ can check every layout, classes with a base
  included. An enum with written values takes the smallest unsigned width
  that holds its largest value.

## The class table and introspection (Hadley, 2026-10-20)

- **Slots**: slot 0 is the destructor (the class's `destroy`, or an empty one
  when it writes none; always virtual, record 0026); then the base's slots in
  its order, an override taking the slot of what it overrides (same name, same
  parameters, record 0020); then the class's new methods in declaration order,
  a generic method's clones included. **`init` takes no slot**: construction
  always calls it directly (question 83). A struct's methods are never slots
  (record 0065), so a class derived from a struct starts its own.
- **No copy of the Itanium vtable**: its two destructor entries, a virtual
  `init` and the RTTI entries are accidents of the emitted C++ or serve nothing
  Haard has, and vtables are the IR's own objects in a whole program.
- **One entry before slot 0 points to the class's descriptor**, where the
  Itanium ABI keeps its RTTI, so the method slots never move for it. The
  descriptor is a read-only IR object (question 81), present only when
  reachable. Version 1: name, size, base -- enough for a checked `downcast` and
  "is it an X?". It is made to **grow**: a field table (name, offset, Haard
  type) and a method table (name, signature, slot) can be added without
  changing what exists. A field's type is the **Haard** type, not the IR's,
  since question 79 took signedness out of the IR.
- **Introspection yes, intercession no.** Reading a program's structure fits:
  statically (the fields of `T` at compile time, what `json(T)` needs, agenda
  1.23; front end only) and dynamically (through the descriptor). Changing it
  while it runs -- adding a method, changing a class -- is out of the language
  on principle: it breaks whole-program compilation (89), devirtualisation (83),
  the closed list of call targets (61) and code placed in scratchpads (81).
  Calling a method by name through the method table is an indirect call, under
  61's rule in real-time code. How introspection is written in Haard is a
  record of its own.

## Stage 1, built (2026-10-20)

In `src/haard/ir/`, with the suite `tests/ir_layout` (20 cases, each rule
proven by sabotage):

- `Target` and `find_target`: the three targets above.
- `IrTypeTable`: one table for the program; scalars seeded, pointers and
  arrays interned, aggregates by identity.
- `IrTypeTranslator`: every Haard type to its IR type, a declaration interned
  by the module and node that declare it (record 0016), a tuple by its IR
  element types, a function value as `{env, call}`.
- `LayoutTable`: the Itanium allocation for single inheritance, per target.
- `IrClassTable`: every class's vtable slots by the rules above, with
  `slot_of` for the lowering of a `vcall`. The descriptor's place is kept; the
  descriptor itself is an object, made with the object table in stage 2.

On x86-64 the suite builds the emitted C++ with a probe and checks that g++
lays out every aggregate exactly as the IR does; it agrees on every case. For
that, the C++ emitter's enum tag moved from `int32_t` to the smallest unsigned
width (record 0043, amended), which changes what `sizeof` gives for an enum in
a program; no exit status of the emitter suite moved.

## Stage 2, built (2026-10-20)

In `src/haard/ir/`, with the suite `tests/ir` (48 cases built by hand with
the builder, every verifier rule proven by sabotage):

- `IrOpcode` and the effects table (question 85), its length checked by a
  `static_assert`, so an opcode without its effects does not compile.
- `IrFunction`: question 91's storage -- typed four-byte ids, fixed-size
  instructions with an operand pool, an index-linked list per block, regions
  owning their blocks, positions as a side table. `IrProgram`: the types, the
  class table, the objects (functions and data, question 81) and interned
  signatures.
- `IrBuilder`, which takes a position on every call (question 86).
- `IrPrinter`, the text above; `IrVerifier`, question 88's rules.

Choices made while building, none of which changes a decision:

- A **slot is an instruction** (`%2 = slot Point`, as LLVM's `alloca`) and not
  a table of its own; question 91's `SlotId` is the instruction's id.
- **Block arguments** go on `br` only; a `cond_br` or a `switch` may not
  target a block with parameters (the verifier says so). Memory form needs
  none; mem2reg will widen this when it comes.
- **Compaction and the alias table** (question 91) wait for the first pass
  that removes or replaces anything, which is stage 3's flatten: no code
  without a case to break it.
- Question 88 listed "8 and 16 bits only in memory" (from 2026-10-08, the
  untyped QBE-level IR). **Dropped** (Hadley, 2026-10-20): `b8` and `b16` are
  values like the others (`add b8` wraps at 8 bits), and each back end
  promotes them to its registers' width, as LLVM's type legalisation does.
  The IR stays as high as possible, and a vectoriser still sees bytes.

## Stage 3, built (2026-10-20)

In `src/haard/ir/`, with 21 more cases in `tests/ir` (69 in all): every
well-formed program is flattened and verified again, and the ones with a
`@main` run.

- `IrFlattener` (question 78): a structured function rebuilt as a CFG. A
  `block` and an `if` get a block after them, a `loop` a header and a block
  after; `break` and `continue` become `br`; values keep their numbers; the
  jumps it writes take their region's position (86). A **loop's body falling
  off its end goes round again**, as Rust's `loop`: the way out is a `break`.
  The header carries the bound (84), and every block knows the innermost loop
  it is in.
- `IrInterpreter` (question 87): memory by object, a pointer the block in
  its high 32 bits and the offset in its low 32; it stops, saying where, on
  an access outside a block, through null, to a slot of a returned frame, a
  write to read-only data, a call through what is not a function, a loop past
  its bound; new slots hold a fixed pattern (10); bytes in the target's order
  by its layout; a `vcall` reads the vptr at the receiver's offset 0 and the
  slot it points into, as selection will write it. Its devices are the
  natives of `std.low_io` (the three handles, `__io_write`, `__io_flush`,
  `__abort`); the file ones stop, saying they are not there yet.
- `IrProgram` gained **natives** (a function object with no body) and
  **relocations** (a datum's bytes holding another object's address), which
  a vtable needs.

What a loop's **bound** counts, made precise here: how many times the loop
may go **back** to its header per entry, by a `continue` or by falling off
the end. A `while i < n` that runs its body n times goes back n times and has
bound n; the last pass, which tests and leaves, is the WCET's "final test".

Not built, and why:

- **Garbage upper bits** (question 87, detail 5) followed the untyped IR of
  2026-10-08, as the 8-and-16-bit rule did: in a typed IR a missing extension
  is a type error the verifier already refuses. Dropped with that rule.
- **Cycle counting** waits for the timing table (62, open).

And one answer the interpreter needed: **a float converted to a whole number
it does not fit, or a NaN, aborts with a message** (Hadley, 2026-10-20,
recommended), as a division by zero does (3a), and for the same reason: a
controller must not act on a value that means nothing. Saturating (Rust,
Java, ARM64 in hardware) was the other choice. `fptosi` and `fptoui` trap in
the effects table, so they are never speculated.

## Stage 4: the lowering, as planned (Hadley, 2026-10-20)

`src/haard/lowering/`: an `IrLowerer` for the program (types, an object per
function, global and literal, vtables as data with relocations, then verify,
flatten, verify) and a `FunctionLowerer` per function (the cleanup stack,
temporaries, construction). Two oracles: every case of `tests/emitter` (and
later `tests/programs`) also runs through the IR, and must give the exit
status its golden already holds, with a list of cases not lowered yet that
fails when one of them starts working; and `tests/lowering`, a small suite
with the IR's text as golden, one construct per case. `hdc --emit-ir` prints
it. Six steps, each closed when its emitter cases agree: (1) functions,
locals, arithmetic, control flow, calls; (2) pointers, references, fixed
arrays, globals, literals, `new`/`delete`; (3) structs, methods,
`init`/`destroy`, the cleanup stack, temporaries; (4) classes, vtables,
`vcall`, `super`; (5) enums and `switch`, tuples, unions, closures, symbols;
(6) the standard library. Choices: every slot in the entry block (question
13's fixed frames); bounds checks (9b) lowered from the start; a position is
the token of the node that made it; the lowering writes only the structured
form; `main` with arguments comes with step 6.

**Step 1 built (2026-10-20)**: `src/haard/lowering/` and `tests/lowering`.
Functions with scalar parameters and results, locals and parameters in
slots at the top of the body (`IrBuilder::entry_slot`), the arithmetic,
bitwise and comparison operators with signedness picked from the Haard type,
`//` (floor) and `**` (a loop, as the C++ path's helper) on whole numbers,
`and`/`or` through a slot, conversions (`i64(x)`, `x as T`), `if`/`elif`/
`else`, `while`, C-shaped `for`, `break`, `continue`, `return`, calls. Four
cases of its own, each checked against the C++ path, and 5 of the 70
emitter programs that have an answer now end the same way through the IR;
65 wait in `not_yet`. `hdc --emit-ir` prints the structured form. The first
emitter program through the IR found a bug in flatten (a dead, empty block
after an inner `if` kept the block after the outer one alive), fixed and
pinned in `tests/ir`. Not in step 1: `**` and `//` on floats, which need the
library's `pow` and `floor`.

## Stage 2: the instruction set and its text (Hadley, 2026-10-20)

The first instruction set:

| Group | Instructions |
|---|---|
| Constants | `const T v`, `null`, `addr @object`, `size T` |
| Integers | `add sub mul`, `sdiv udiv srem urem`, `smulhi umulhi`, `and or xor`, `shl lshr ashr`, `neg not` |
| Floating point | `fadd fsub fmul fdiv fneg` |
| Comparisons | `eq ne slt sle sgt sge ult ule ugt uge`, `feq fne flt fle fgt fge` |
| Conversions | `trunc zext sext`, `sitofp uitofp fptosi fptoui`, `fpext fptrunc`, `ptrtoint inttoptr` |
| Choice | `select` |
| Memory | `slot T`, `load T`, `store T`, `copy T`, `field`, `index`, `upcast`, `downcast` |
| Calls | `call @f`, `call_indirect`, `vcall Class.method` |
| Barriers (85) | `volatile_load`, `volatile_store`, `fence` |
| Structured (78) | `block`, `if`/`else`, `loop` (with its bound), `break $r`, `continue $r` |
| CFG | `br`, `cond_br`, `switch`, with block parameters |
| Leaving | `ret`, `abort "message"` |

Timing instructions (64) and transfers wait for the hardware; the effects
table already has their class. And five choices:

1. **Haard's semantics is on the instruction, with no undefined behaviour**:
   `add` wraps (1a), a shift by the width or more gives the arithmetic result
   (2b), `sdiv` by zero aborts and `MIN / -1` is `MIN` (3a). Each back end
   writes the check its machine needs. The IR has no `undef` and no `poison`
   (10), and no optimiser reasons about undefined behaviour.
2. **No `for` region yet**: the lowering makes `loop`s until loop passes and
   the counted loop come (82).
3. **`abort` is a terminator**, like `ret`, for `unwrap` and the bounds checks
   (9b); its message is a read-only object.
4. **A value is visible after its definition, in its region and the regions
   inside it**, and never leaves a region: what has to goes through a slot.
   Dominance in the structured form is a scope rule.
5. **Text**: `%n` a value, `@name` an object, `$name` a region, `^name` a CFG
   block, the type written on the instruction (80).

## Consequences

- The order of the work in the notes (section 11) starts: stage 1 is the
  IR's types, the layout and class tables, and the target description.
- The open questions (25-77) touch only the processor's back end and the
  real-time rules; the three reservations above keep the IR's shape the
  same whatever they answer. The first real back end is x86-64 or ARM64.
- The language questions among them (block E, real time) become records of
  their own when answered; the rest amend this one.
