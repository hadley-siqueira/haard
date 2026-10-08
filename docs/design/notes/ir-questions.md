# Questions to settle before building the IR

Written 2026-10-08. The complete list, numbered once; it replaces every
earlier list given in conversation. The reasoning behind each block is in
[designing-the-ir.md](designing-the-ir.md), and the decisions already taken
are record [0077](../0077-an-ir-for-the-processor.md).

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
2. **Shift by a count ≥ the width** (or negative): (a) the count is masked to
   the width; (b) the result is 0 or the sign fill; (c) aborts.
   **Recommended (a)**.
3. **Integer division by zero, and `MIN / -1`**: (a) aborts with a message;
   (b) undefined; (c) a defined value. **Recommended (a)**.
4. **Exceptions with unwinding**: (a) never, errors are values and the rest
   aborts; (b) someday. **Recommended (a)**.
5. **Aliasing between pointers and `T&`**: (a) they may always overlap; (b)
   `T&` parameters never overlap; (c) (a) plus an opt-in annotation, like C's
   `restrict`. **Recommended (c)**.
6. **Type-based aliasing** (C's strict aliasing): (a) none, any pointer may
   reach anything; (b) as in C. **Recommended (a)**.
7. **Floating point**: (a) strict IEEE always; (b) strict, with opt-in
   relaxation (reassociation, FMA contraction) per function or block; (c)
   relaxed by default. **Recommended (b)**.
8. **Rounding mode**: (a) fixed, to nearest; (b) changeable by the program.
   **Recommended (a)**.
9. **Array bounds checks**: (a) never; (b) always, aborting; (c) only in a
   debug mode. **Recommended (c)**.
10. **Reading uninitialised memory**: (a) an unspecified value with no other
    effect; (b) undefined, as in C. **Recommended (a)**.
11. **Memory-mapped registers in Haard**: (a) `volatile` on a pointer type;
    (b) intrinsic functions; (c) only compiler-generated. **Recommended (b)**.
12. **New numeric types**: which of `f16`, `bf16`, `i128`/`u128`, or none?
13. **Dynamic stack allocation**: (a) none, frames have a fixed size; (b)
    allowed. **Recommended (a)**, firmly: worst-case stack use stays
    computable.
14. **Guaranteed tail calls**: (a) no, only an optimisation; (b) guaranteed
    when written. **Recommended (a)**.

## B. The processor

15. **Registers**: how many general purpose? A separate floating point bank?
    A vector bank?
16. **Can the dedicated stack pointer be the base of a load or store**
    (`[sp + 16]`), or must it be copied into an ordinary register first?
17. **Instructions that touch the stack pointer**: push/pop? add an immediate
    (how many bits)? copy SP ↔ register?
18. **Frame pointer**: dedicated, an ordinary register by convention, or none?
19. **Return address**: (a) a link register (dedicated or ordinary?); (b)
    pushed by the call instruction.
20. **Other dedicated registers**: zero? flags (does a compare write flags or
    a register)? a readable PC? global pointer? thread pointer?
21. **Stack**: growth direction, alignment, hardware overflow detection?
22. **Encoding**: fixed length (how many bits?), variable or compressed? Bits
    of the load/store, arithmetic and branch immediates?
23. **Pipeline**: (a) thread-interleaved (as PTARM / FlexPRET); (b) scalar
    in order; (c) VLIW; (d) other. Exposed pipeline (delay slots) or
    interlocks?
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
