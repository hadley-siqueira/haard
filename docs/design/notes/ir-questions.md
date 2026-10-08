# Questions to settle before building the IR

Written 2026-10-08. The complete list, numbered once; it replaces every
earlier list given in conversation. The reasoning behind each block is in
[designing-the-ir.md](designing-the-ir.md), and the decisions already taken
are record [0077](../0077-an-ir-for-the-processor.md). The explanation given
for each question, before it was answered, is kept in
[ir-questions-explained.md](ir-questions-explained.md).

Answer by number. Where a question carries a **Recommended** option and there
is no preference, the recommendation stands. "Open" is a valid answer for a
hardware question whose answer is not known yet: those touch only the back
end, and the IR can start without them. As answers arrive they are written
here, under the question, and the language decisions get records of their
own.

The ones that weigh most on the design: **1–5, 13, 16, 19, 23, 32, 35, 38,
39, 49, 56–58, 61 and 79–81**.

## A. Language semantics (what each IR instruction means)

1. **Signed integer overflow**: (a) wraps; (b) aborts; (c) undefined, the
   optimiser may assume it never happens. **Recommended (a)**; counted loops
   state their trip count by structure, so (c) would buy little. Unsigned
   always wraps.

   **Answer (Hadley, 2026-10-08): (a)**, wraps.
2. **Shift by a count ≥ the width** (or negative): (a) the count is masked to
   the width; (b) the result is 0 or the sign fill; (c) aborts.
   **Recommended (a)** at first.

   **Answer (Hadley, 2026-10-08): (b)**, the arithmetic result, after the
   recommendation was revised to (b). Masking (Java, C#, JavaScript; what
   x86-64, ARM64 and RISC-V do in hardware) makes `(1 << n) - 1` give 0 for
   `n = 32` instead of all ones; Go and Swift chose the arithmetic result.
   So: `x << n` with `n` ≥ the width is 0; `x >> n` is 0, or the sign fill
   for a signed `x`; a **negative** count counts as too large (same
   results, never an abort); a **constant** count ≥ the width is a compile
   error. Cost: about two instructions (compare, select) only when the
   count is not constant, constant in time; the processor's own shift may
   implement the rule directly.
3. **Integer division by zero, and `MIN / -1`**: (a) aborts with a message;
   (b) undefined; (c) a defined value. **Recommended (a)**.

   **Answer (Hadley, 2026-10-08): (a)**, with `MIN / -1` giving `MIN`.
   Applies to `/`, `//` and `%` on whole numbers (floating point keeps
   IEEE: infinity and NaN). A zero divisor aborts with a message, checked
   only when the divisor is not a constant; a constant zero divisor is a
   compile error. `MIN / -1` wraps to `MIN` and `MIN % -1` is 0, as
   overflow wraps (answer 1) -- what Java and Go do and what ARM64 and
   RISC-V give for free; x86-64 traps on it, so its back end checks a
   non-constant signed divisor for -1. Why abort and not a value: on ARM
   and RISC-V a division by zero silently yields 0 or -1, and a
   cyber-physical controller would act on it.
4. **Exceptions with unwinding**: (a) never, errors are values and the rest
   aborts; (b) someday. **Recommended (a)**.

   **Answer (Hadley, 2026-10-08): (a)**, never. Expected errors are values
   (`Option`, enums, a future `Result<T, E>` with propagation sugar, which
   needs nothing from the IR); what should not happen aborts. So the IR has
   no `invoke`, no landing pads, no unwind tables, one cleanup path per
   scope, and no unwinder in any back end or in the interpreter. Why: table
   unwinding nearly doubles the CFG's edges, costs thousands of cycles per
   throw with a hard-to-bound time (JSF AV C++ forbids exceptions), takes
   scratchpad space, and adding it later is the most invasive change a
   language can make. What "abort" does on the processor (a safe-mode
   routine?) belongs to question 29.
5. **Aliasing between pointers and `T&`**: (a) they may always overlap; (b)
   `T&` parameters never overlap; (c) (a) plus an opt-in annotation, like C's
   `restrict`. **Recommended (c)**.

   **Answer (Hadley, 2026-10-08): (a)**: any two pointers or references may
   overlap, and there is no annotation -- Hadley does not want a new keyword
   for it. Settled on the way: aliasing only matters when one side is
   **written**; a function that only reads through its parameters is
   optimised freely, so `v.dot(v)`, `a == a` and a `mul(m, m)` returning a
   new matrix lose nothing. Fortran's rule is exactly that (overlap is
   forbidden only when one of the overlapping arguments is modified), which
   an earlier explanation overstated. Rejected (b) because the written
   overlaps it would make undefined are common in Haard, where `T&` is also
   the don't-copy convention: `xs.push(xs[0])` (push writes self and may
   reallocate the buffer the element lives in), `list.append_all(list)`,
   `m.mul_in_place(m)`. Consequences: the optimiser proves non-overlap
   itself (distinct locals, globals, `new` results, and two distinct
   `Array` objects, which own their buffers -- record 0031); where it
   cannot, a vectoriser or a scratchpad/CGRA transfer needs a run-time
   overlap test with a scalar fallback, whose duplicated code costs
   scratchpad space and whose slower path is what the WCET counts.
6. **Type-based aliasing** (C's strict aliasing): (a) none, any pointer may
   reach anything; (b) as in C. **Recommended (a)**.

   **Answer (Hadley, 2026-10-08): (a)**: no type-based aliasing rule.
   Reading memory as another type (a union, record 0064; a pointer cast,
   record 0049; a byte buffer read as a struct; device registers) is always
   legitimate, and no load or store in the IR carries type information for
   aliasing (no TBAA). Why: C's rule makes exactly that code undefined (the
   Linux kernel builds with `-fno-strict-aliasing`), and the gain is small
   in Haard: a `for` over a range evaluates its bound once (checked: `for
   i in 0...*n` copies `*n` before the loop), and whole-program analysis
   proves much of the rest. What remains are `while` loops that re-read a
   field while writing through a pointer: one reload per iteration, or a
   run-time test before vectorising. Coherent with answer 5.
7. **Floating point**: (a) strict IEEE always; (b) strict, with opt-in
   relaxation (reassociation, FMA contraction) per function or block; (c)
   relaxed by default. **Recommended (b)** at first, then **(a)** once
   reproducibility across targets was weighed.

   **Answer (Hadley, 2026-10-08): (a)**, strict always. Nothing is
   reordered or contracted; `fma(a, b, c)` is a library function and an IR
   instruction emitted only when written; reductions are vectorised by hand
   with explicit vector types (question 47). The C++ path must be built
   with `-ffp-contract=off` so both sides of the oracle agree bit for bit.
8. **Rounding mode**: (a) fixed, to nearest; (b) changeable by the program.
   **Recommended (a)**.

   **Answer (Hadley, 2026-10-08): (a)**, fixed: round to nearest, ties to
   even. Every floating-point operation is a pure function of its operands;
   constants fold at compile time with the run-time result. Interval
   arithmetic, if ever needed, comes as library functions that fix the mode
   on one operation (`add_down`, `add_up`), an attribute on the IR
   instruction, never a global state.
9. **Array bounds checks**: (a) never; (b) always, aborting; (c) only in a
   debug mode. **Recommended (c)** at first, then **(b)**.

   **Answer (Hadley, 2026-10-08): (b)**, always checked, aborting with a
   message. Fixed arrays are checked by the compiler; `Array`, `List` and
   `String` check in their own Haard `operator[]` (record 0034) through an
   intrinsic that becomes a dedicated IR instruction (`check_index %i,
   %size`), so the optimiser can eliminate it (a `for` over a range proves
   the index) or hoist it before a loop, and the WCET sees a path that does
   not return. Indexing a raw pointer (`T*`, `new T[n]`) is never checked:
   that is the unchecked way out, with no keyword.
10. **Reading uninitialised memory**: (a) an unspecified value with no other
    effect; (b) undefined, as in C. **Recommended (a)**.

    **Answer (Hadley, 2026-10-08): (a)**: reading memory nobody wrote
    (fields `init` left alone, record 0026; `new T[n]` buffers, record 0028;
    an `Array`'s spare capacity) gives some bit pattern, the same one on
    every read until a write, and has no other effect. The IR has no
    `undef` and no `poison`; mem2reg may use a fixed constant (0) for a slot
    read before any store. The interpreter gets a mode that fills new
    memory with a garbage pattern, to catch programs that depend on it.
    Zeroing everything (Go, Java) stays rejected by record 0026.
11. **Memory-mapped registers in Haard**: (a) `volatile` on a pointer type;
    (b) intrinsic functions; (c) only compiler-generated. **Recommended (b)**.

    Noted by Hadley (2026-10-08), before this question was discussed:
    Haard has no `volatile` yet and it must be implemented in the future.
    This question decides how it is spelled.

    **Answer (Hadley, 2026-10-08): (b)**, intrinsic functions in the
    library: `volatile_read<T>(p : T*) -> T`, `volatile_write<T>(p : T*, v :
    T)` and `fence()`, with `T` limited to whole numbers of 8, 16, 32 and 64
    bits (the width must be exact). No keyword and no change to the type
    system; the compiler recognises the functions and emits volatile loads
    and stores, which are full barriers in the IR (question 85) and visible
    to the WCET. A `Register` class can be built on top in the library. A
    type qualifier stays possible later, additively. Not implemented yet.
12. **New numeric types**: which of `f16`, `bf16`, `i128`/`u128`, or none?

    **Answer (Hadley, 2026-10-08): none for now.** The IR's type
    representation stays open (integers of any width, floating-point formats
    as an extensible list) so adding one later is cheap; adding a numeric
    type changes no existing program. `f16`/`bf16` are to be reconsidered
    with SIMD and the CGRA (questions 46-48, 51), where they pay; `bf16`
    would need rounding rules of Haard's own to respect answer 7. The main
    use of 128-bit integers, the full product of two `u64`, comes as a
    library function returning a tuple: `let (high, low) = mul_wide(a, b)`.
13. **Dynamic stack allocation**: (a) none, frames have a fixed size; (b)
    allowed. **Recommended (a)**, firmly: worst-case stack use stays
    computable.

    **Answer (Hadley, 2026-10-08): (a)**: every stack frame has a size
    known at compile time. The compiler computes each thread's worst-case
    stack use; the IR never names the stack pointer (every slot is static
    and the back end sizes the frame); no function needs a frame pointer for
    this. Nothing in Haard needed it (every type has a fixed size, tuples
    and closure environments included), so no program changes. Variable
    buffers use a fixed maximum capacity, or a time-bounded pool
    (question 60).
14. **Guaranteed tail calls**: (a) no, only an optimisation; (b) guaranteed
    when written. **Recommended (a)**.

    **Answer (Hadley, 2026-10-08): (a)**: tail calls are only an
    optimisation. The machine-level optimiser may turn a call in tail
    position into a jump when it is safe (no pending cleanups, no local lent
    by reference, a compatible signature); programs may not rely on it. The
    stack analysis runs on the final code, so it sees the effect. An
    explicit form (as Zig's `@call(.always_tail, ...)`) stays possible
    later, additively.

## B. The processor

15. **Registers**: how many general purpose? A separate floating point bank?
    A vector bank?

    **Answer (Hadley, 2026-10-08): partly open.** The general-purpose
    count is still **16 or 32**. With 32, the design follows RISC-V and ARM.
    With 16, there are 16 general-purpose registers **plus dedicated ones**:
    stack pointer, frame pointer, thread pointer, instruction pointer and
    link register, probably close to the address generation unit. Floating
    point has a **separate bank** (likely 16 with 16 GPRs). The vector bank
    (separate, or shared with floating point) is not decided. None of this
    touches the IR; the back end needs the count per bank, the reserved
    registers (question 20) and the calling convention (question 31).
16. **Can the dedicated stack pointer be the base of a load or store**
    (`[sp + 16]`), or must it be copied into an ordinary register first?

    **Answer (Hadley, 2026-10-08): the ARM64 model.** SP, FP, TP and IP
    are usable as the **base of a load or store** with an immediate offset
    (`[sp + 16]`, `[fp + 8]`, `[tp + 4]`, `[ip + 64]` for globals and
    constants near the code), and an instruction computes `register = SP/FP
    + immediate`, so taking the address of a local (a `T&` to it) is one
    instruction. No general-purpose register is spent on a copy of SP.
17. **Instructions that touch the stack pointer**: push/pop? add an immediate
    (how many bits)? copy SP ↔ register?

    **Answer (Hadley, 2026-10-08): the ARM64 model**, and the architecture
    will probably support every operation listed in the explanation:
    `sp = sp ± immediate` (at least 12 bits), `sp = sp - register` for
    larger frames, `register = sp/fp + immediate`, `fp = sp + immediate`
    and `sp = fp + immediate`, `register = sp` and `sp = register` (start-up,
    thread stacks, overflow checks; writing SP may be privileged), and store
    / load pair with pre-decrement and post-increment (or multi-register
    push / pop) for compact prologues and epilogues. Every instruction's
    effect on SP outside start-up and thread creation is a constant, as the
    stack analysis needs.
18. **Frame pointer**: dedicated, an ordinary register by convention, or none?

    **Answer (Hadley, 2026-10-08): keep the frame record always.** The
    hardware side follows answer 15 (a dedicated FP with 16 general
    registers; a general register by convention, as ARM64's x29, with 32).
    The compiler always saves the pair old FP + return address and points FP
    at it, so the frames form a linked list: the abort routine can print the
    call history with no unwind tables (there are none, answer 4), and
    debuggers and profilers walk the stack the same way. Cost: two or three
    instructions per call with store pair (answer 17), fixed in the WCET.
    The suggested exceptions (leaf functions, an opt-out option) were not
    taken: always means always.
19. **Return address**: (a) a link register (dedicated or ordinary?); (b)
    pushed by the call instruction.

    **Answer (Hadley, 2026-10-08): the ARM64 model**: (a), a link register
    (dedicated with 16 general registers, a general one by convention with
    32). A call writes LR (`bl`), `ret` jumps to it, a leaf function never
    touches memory for it, and a non-leaf saves it with FP as the frame
    record (answer 18) in one store pair (answer 17). With it: a call
    through a register (`blr`, needed by `vcall`, closures and bound
    methods), a separate register for an interrupt's return address (as
    ARM's ELR, question 65), and a direct-call range covering a scratchpad's
    code, with veneers for farther calls (question 22).
20. **Other dedicated registers**: zero? flags (does a compare write flags or
    a register)? a readable PC? global pointer? thread pointer?

    **Answer (Hadley, 2026-10-08):** **no zero register**; a **dedicated
    branch that compares with zero**; branches in the **RISC-V / MIPS
    style** (compare and branch in one instruction, no condition flags for
    branching); the architecture **has add with carry**; **GP is a dedicated
    register**, like SP and FP. A readable PC (IP, usable as a base) and a
    dedicated TP were already settled by answers 15 and 16. Open detail:
    where the carry of an add-with-carry lives (a single carry bit, or a
    register).
21. **Stack**: growth direction, alignment, hardware overflow detection?

    **Answer (Hadley, 2026-10-08):** the stack grows **down**; SP is
    aligned to **16 bytes**, checked by the hardware as on ARM64 (to be
    revisited if the SIMD registers are wider than 128 bits, question 46,
    before the ABI freezes); overflow is caught by a **stack-limit register
    per hardware thread** (as ARMv8-M's `MSPLIM` / `PSPLIM`), no
    instructions spent. Real-time code is proven not to overflow at compile
    time; the limit register is the safety net, and an overflow aborts with
    a message through the abort routine (question 29).
22. **Encoding**: fixed length (how many bits?), variable or compressed? Bits
    of the load/store, arithmetic and branch immediates?

    **Answer (Hadley, 2026-10-08): instructions of 32 or 16 bits** (a
    compressed form beside the 32-bit one). Immediates: **12 or 16 bits,
    still open; at least 12**. Still open after the answer: the ranges of
    conditional branches and of calls/jumps; whether load/store offsets are
    scaled by the access size; whether a 32-bit instruction may start at a
    2-byte boundary (and so cross a fetch word, which matters to the WCET);
    which registers and immediates the 16-bit forms reach.
23. **Pipeline**: (a) thread-interleaved (as PTARM / FlexPRET); (b) scalar
    in order; (c) VLIW; (d) other. Exposed pipeline (delay slots) or
    interlocks?

    **Answer (Hadley, 2026-10-08): thread-interleaved, and a thread may
    fetch more than one instruction in parallel.** The **compiler**
    guarantees that instructions issued together can run in parallel; each
    instruction has a **dedicated bit**, set by the compiler, saying whether
    it runs in parallel with its neighbour (as the p-bit of TI's C6000 VLIW
    or Hexagon's packets). So the back end has a scheduler/bundler and the
    hardware does not check the bundles. A correct first back end may clear
    the bit everywhere (fully sequential); bundling is then an optimisation.
    Open details: the maximum instructions per bundle; which combinations
    are allowed (functional units: how many memory, branch, FP per bundle);
    the semantics inside a bundle (all reads before any write?); whether one
    thread can issue bundles in consecutive cycles (interlocks between
    bundles?); whether 16- and 32-bit instructions mix in one bundle.
24. **Conditional execution**: a select or conditional move? Predicate
    registers?
25. **Multiply and divide in hardware?** Fixed latency? Division by zero:
    trap or a value?
26. **Unaligned access**: allowed, or a trap?
27. **Floating point**: `f32` and `f64`? FMA? Denormals or flush to zero?
    Fixed latencies?
28. **Environment**: bare metal? An MMU? Entry point and memory map?
29. **Output and ending**: how does a program write a character (a UART
    mapped in memory?), exit, and abort?
30. **64-bit constants**: loaded how (several instructions, a pool, an
    instruction of their own)?
31. **The processor's ABI**: (a) proposed by me once the ISA settles; (b)
    defined by you.

## C. Memory: scratchpads and overlays

32. **Which memories exist**: instruction scratchpad(s), data scratchpad(s),
    main memory, flash or ROM? Their sizes?
33. Is each scratchpad **private to a hardware thread** or shared?
34. Can code **execute from main memory**, or only from an instruction
    scratchpad?
35. Does the ISA reach every memory **with the same load/store instructions**
    (by address), or with **different instructions** per memory (as Patmos
    does)? (The second puts an address space in the pointer type.)
36. **Latencies**: fixed per memory? Is main memory predictable (a PRET
    memory controller)?
37. **DMA**: is there an engine? Asynchronous (start, then wait)? Is its
    timing predictable?
38. **Who decides placement**: (a) the compiler, automatically; (b) the
    programmer, by annotation; (c) the compiler, with the programmer able to
    force it. **Recommended (c)**.
39. **A program larger than the scratchpads**: (a) refused, it must fit; (b)
    static overlays planned by the compiler; (c) a software cache at run
    time. **Recommended (a) in the first version, then (b)**; (c) is
    unpredictable.
40. **What may be overlaid**: **Recommended**: code first, then data whose
    address does not escape.
41. **The stack in a scratchpad**, one per hardware thread, sized by the
    worst-case stack analysis? **Recommended yes**.
42. **The heap**, if any: where does it live?
43. **Arrays larger than a scratchpad**: (a) the programmer moves the pieces
    through a library; (b) the compiler tiles the loop with DMA.
    **Recommended (a) first, (b) later**.
44. **Start-up**: who copies code and data into the scratchpads: the
    hardware, a boot ROM, or a loader the compiler emits?
45. **Caches**: are there any in some versions? If so, which, and with what
    replacement policy (LRU is the analysable one)?

## D. SIMD and the CGRA

46. **SIMD width**: (a) fixed (which?); (b) length agnostic (as ARM SVE or
    RISC-V V).
47. **SIMD in the language**: (a) explicit vector types; (b) only
    automatic; (c) both. **Recommended (c), explicit first**.
48. **What SIMD supports**: element types, per-lane masks, gather/scatter?
49. **How the CGRA is coupled**: (a) an accelerator configured through MMIO or
    DMA, started and waited for; (b) inside the pipeline, with instructions
    of its own.
50. **The CGRA's memory**: its own scratchpad, main memory, DMA?
51. **The CGRA's operations**: integer, floating point, which set?
52. **Who produces the CGRA configuration** (mapping, modulo scheduling): (a)
    hdc; (b) a tool of yours, hdc handing it the kernel's dataflow graph.
53. **Who chooses what goes to the CGRA**: (a) the programmer marks it; (b)
    the compiler; (c) both. **Recommended (a) first**.
54. **Is a kernel's time on the CGRA predictable**, so it can enter the WCET?
    Fixed once mapped, or data dependent?

## E. Real time

55. **Which analyses**: WCET per function or task, worst-case stack use,
    others? (Schedulability across tasks usually sits outside the compiler,
    taking the WCET as input.)
56. **Timing constructs in Haard**: (a) intrinsics or a library (`now()`,
    `delay_until(t)`, deadlines); (b) syntax of their own (in the style of
    Lingua Franca, from the PRET group); (c) none, analysis only.
    **Recommended (a) now**.
57. **Where the real-time restrictions apply**: (a) the whole program when
    targeting the processor; (b) only marked functions or tasks and what they
    call. **Recommended (b)**.
58. **Loop bounds**: (a) inferred from `for` over a constant range, an
    annotation required otherwise (an error in real-time code without one);
    (b) always annotated. **Recommended (a)**; the annotation's spelling gets
    a record of its own.
59. **Recursion in real-time code**: (a) forbidden; (b) allowed with an
    annotated depth. **Recommended (a)**.
60. **Heap in real-time code**: (a) forbidden; (b) allowed with a
    time-bounded allocator (pools). **Recommended (a) now, (b) later**.
61. **Indirect calls in real-time code** (virtual, closure, function value):
    (a) allowed when whole-program analysis finds every target, annotated
    otherwise; (b) forbidden. **Recommended (a)**.
62. **The timing model**: does every instruction take a fixed number of
    cycles? Branches a fixed cost? Anything data dependent (divide,
    multiply)?
63. **Hardware threads**: how many? Does each run a separate Haard task? How
    do they share memory and communicate?
64. **Timing instructions in the ISA**: which? `get_time`, `delay_until`, an
    exception or interrupt when a deadline passes?
65. **Interrupts**: do they exist? Are handlers written in Haard (a marked
    function that saves every register)? How do they enter the WCET?
66. **Single-path / constant-time code**: an option that transforms a
    function so it always takes the same time (branches into predication)?
    Depends on 24.
67. **Optimisations on the processor**: (a) all of them, the WCET computed
    afterwards, flow facts kept by contract; (b) only those that do not
    hinder the analysis; (c) WCET-guided optimisation. **Recommended (a) now,
    (c) much later**.
68. **Report**: WCET per function with the worst path in source lines?
    **Recommended yes**.
69. **The interpreter counts cycles** with the timing table, and the tests
    check measured ≤ WCET? **Recommended yes**.
70. **Real-time code on x86/ARM**: (a) the restrictions are checked on every
    target, being language rules; (b) only on the processor.
    **Recommended (a)**.

## F. Targets and tools

71. **x86-64 and ARM**: (a) assembly text and the system's `as`/`ld`; (b) our
    own assembler and linker. **Recommended (a) first**.
72. **The processor**: is there an assembler already, or does hdc produce the
    binary directly?
73. **Operating systems on x86/ARM**: (a) Linux only; (b) plus macOS; (c)
    plus Windows; (d) plus Android. **Recommended (a) first**.
74. **Interoperating with C**, following each platform's ABI: yes or no?
    **Recommended yes**.
75. **Run time on x86/ARM**: (a) libc; (b) direct system calls.
    **Recommended (a)**.
76. **DWARF debug information**: later, with source positions per
    instruction kept from day one? **Recommended yes, later**.
77. **Position-independent executables on Linux**: **Recommended yes**.

## G. The IR's own design (proposed; approve or overturn)

78. **One IR, two control forms** (structured and CFG), with a flatten pass,
    instead of two IRs.
79. **Typed values**, signedness in the type (`i32` ≠ `u32`).
80. **Typed pointers, fields by name, elements by index**; layout, classes
    and vtables in explicit tables, the layout computed per target.
81. **Every object in memory is a named entity** (function code, global,
    literal, vtable, stack); instructions refer to it by symbol; **placement
    is a table** applied at final layout, so it can be recomputed.
82. **Locals**: a name never rewritten and whose address is never taken is a
    value; the rest get a slot. mem2reg later.
83. **`vcall` and the classes stay until instruction selection**;
    destructors and temporaries become explicit calls through a cleanup
    stack.
84. **Flow facts in the IR**: a bound on each loop region, kept by every
    pass, checked by the verifier.
85. **Timing instructions, volatile accesses and memory transfers (copy,
    DMA) are barriers** to the optimiser and visible to the WCET.
86. **Source position per instruction** and a machine ↔ IR ↔ source mapping.
87. **The interpreter runs the CFG form**, with endianness, layout, timing
    table and memory map (including overlay residency) as parameters.
88. **The verifier** runs after the lowering and every pass, always in the
    tests.
89. **Whole-program compilation**.
90. **WCET computed by hdc** on the processor's final machine IR.
91. **Plain data by index, stored per function**, side tables keyed by id,
    stable ids inside a pass and compaction between passes.
