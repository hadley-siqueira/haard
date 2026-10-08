# Designing the intermediate representation

Written 2026-10-07 and 2026-10-08, over a long conversation with Hadley before
any IR code exists. It keeps the whole road: every proposal, why it was
replaced, what each target and goal asks of the IR, and what is still open.
The decisions taken so far are record
[0077](../0077-an-ir-for-the-processor.md); the questions still waiting for
an answer are numbered in [ir-questions.md](ir-questions.md).

Hadley's standing instruction for this design (2026-10-08): **aim for the best
compiler, even if it overturns an earlier decision of his**; make the IR **as
high level as possible** (a field read by name rather than by offset, when
lowering it later is trivial); explicit tables for classes, structs and
layout are welcome; and the storage model (plain data by index) may change if
it gets in the way.

## 1. Why an IR now

Record [0025](../0025-the-back-end-is-a-cpp-transpiler.md) chose no IR for the
C++ back end and named three signals that would make that the wrong shape.
Two have arrived:

- **A target with real distance.** Hadley's processor, and native x86-64 and
  ARM.
- **Work C++ did in silence.** Destructor calls at every scope exit and
  temporaries at the end of a statement (the largest part), copy `init` and
  assignment calls, vtables and the vptr, the layout of fields, unions, enum
  tag and payload, tuples, closures as environment plus function, and
  record 0068's temporaries as real locals.

Generics are already concrete clones (record 0002), so no IR ever sees a `T`.
The C++ back end **stays**: any machine with a C++ compiler is a target, which
is portability no native back end gives.

## 2. Targets and goals

Collected from Hadley over the two days:

| | |
|---|---|
| His processor | 64-bit, **big endian**, floating point, ISA still being designed |
| It is a **PRET** machine | Precision Timed: built for cyber-physical and real-time systems; he needs **WCET** and the other real-time analyses |
| Memory | **Scratchpads**, often **no cache**: the compiler chooses where code and data live, possibly with **overlays** |
| Registers | Some are **dedicated**: the stack pointer, for one, is a special register reached by its own instructions |
| Later | **SIMD**, and integration with a **CGRA** (coarse grained reconfigurable array) |
| Also native | **x86-64 and ARM (AArch64)**, little endian: laptops, desktops, phones |

## 3. The road taken

Four proposals, in order. Each was replaced for a stated reason; the reasons
are the useful part.

### 3.1 LLVM's shape (2026-10-07)

An IR of our own shaped like LLVM's: functions of basic blocks, typed
three-address instructions, explicit terminators, opaque pointers with
`field_address` / `element_address`, an interned IR type table. Memory form
first (every local an `alloca`, every access a `load` / `store`, as
`clang -O0`), SSA later through a `mem2reg` pass. An interpreter as the test
oracle; the text a read-only dump.

Hadley's answers then: **memory form first, SSA later; the interpreter as the
oracle; the text only a dump.** These three survive every later proposal.

### 3.2 QBE's level (2026-10-08)

Hadley asked whether an IR closer to assembly would be simpler: every virtual
register 64 bits, the width in the opcode (`add32`, `add64`, loads by width).
That is QBE's design (used by Hare and cproc): four temporary classes
`w`/`l`/`s`/`d`, memory widths in the load and store names, no structs (a
field is `add base, 8` then a load), and its slogan of "70% of the
performance of advanced compilers in 10% of the code".

It raised the question of **what a 32-bit operation leaves in the upper half
of a 64-bit register**, and how real machines answer it:

| Family | Machines | Upper 32 bits after a 32-bit op |
|---|---|---|
| Zero | x86-64 (but 8/16-bit writes **preserve** the rest), AArch64 (`Wn` writes zero `Xn`'s top) | zeroed; unsigned widening free, signed costs `movsxd` / `sxtw` (or an extended operand on ARM) |
| Sign | MIPS64 (inputs not sign-extended are UNPREDICTABLE), RISC-V RV64 (`addw` & co., even `divuw`), Alpha | sign-extended; signed widening free; sign extension preserves both signed and unsigned order, so 64-bit compares serve both |
| Don't care | SPARC V9 (`icc`/`xcc` flags, 32- and 64-bit right shifts), POWER (`cmpw`/`cmpd`, `extsw`) | whatever the 64-bit op produced; 32-bit compares and shifts exist; ABIs extend at call boundaries |

**Decided by Hadley: upper bits undefined** in the IR, widening explicit, 8
and 16 bits only in memory. It is the only choice that lowers well to all
three families. An idea that came with it: the interpreter can fill upper
bits with garbage in a test mode, so a missing extension changes an exit
status.

Hadley then asked why a register needs a class at all. Answer: only the
allocator needs one, and only to pick a **bank** (integer or float); it can
be derived from the defining opcode if every value-producing instruction
names its bank (`copy`, `phi`, `load`, `call`, parameters), with a verifier
checking that every use agrees with its definition.

**Superseded by 3.4.** Once a table of structs, classes and layout exists
anyway, types on values cost little and give a verifier for free; the
upper-bits question then belongs to each back end, not to the IR. Hadley
confirmed he only suggested it to simplify, and it does not.

### 3.3 Two levels (2026-10-08)

SIMD and the CGRA changed the picture. Vectorising a loop, or mapping it onto
a CGRA, needs to know that it is a loop, its induction variable and trip
count, that accesses are contiguous with a known stride, and that two arrays
do not overlap. At QBE's level the loop is a graph of blocks and `a[i]` is
`base + i*4`; all of it has to be **recovered** by analysis (LLVM's
ScalarEvolution and dependence analysis, among its heaviest parts). A CGRA
flow (find the kernel, build its dataflow graph, check loop-carried memory
dependences, modulo-schedule, map, emit host code) also makes **SSA
mandatory** for the kernel.

The evidence from the field: **MLIR** was created at Google because LLVM IR
is too low for accelerators, and most CGRA and high-level-synthesis work now
uses multi-level IRs with structured, affine loops on top.

So: a structured, typed, target-independent **HIR** above the QBE-level
**LIR**. **Superseded by 3.4**: with the LIR typed and reading fields by name,
the only real difference between the two is the shape of control flow, and
two whole IRs (two builders, printers, verifiers) would be duplication.

### 3.4 One IR, two control forms (current proposal)

One instruction set, one type table, one printer, one verifier. Control flow
comes in two forms:

- **Structured**: regions `if`, `loop`, `for`, `block`, with `break` and
  `continue` (as in WebAssembly). The lowering produces this form. Loop
  transformations, vectorisation, CGRA kernel extraction, function splitting
  and real-time flow facts work here.
- **CFG**: basic blocks with **block parameters** and `br` / `cond_br` /
  `switch`. mem2reg, scalar optimisations and instruction selection work
  here.

A **flatten** pass turns the first into the second; WebAssembly shows this is
easy. Below it, each back end has its own **machine IR** (real instructions,
physical registers, spills, prologue and epilogue, bundles on a VLIW).

## 4. The current proposal in detail

### Types and tables

- Values are typed: `bool`, `i8`…`i64`, `u8`…`u64`, `f32`, `f64`, `ptr<T>`,
  `vec<N x T>`, and structs and classes by name. **Signedness is in the type**
  (Haard's model), so `div` picks signed or unsigned from its operands.
- Pointers are **typed**; a field is reached by name (`field %p, Point.x`)
  and an element by index (`index %p, %i`). Offsets never appear in
  instructions.
- Explicit tables: structs and their fields, classes with their base and
  vtable, unions, enum tag and payload, and a **layout per target** (size,
  alignment, field offsets, endianness). Instruction selection reads the
  layout table; nothing above it does.

### Values, slots and SSA

- A source name that is never written after its definition and whose
  address is never taken is a **value**; any other gets a **slot** (memory
  form). The front end already knows both facts, so mem2reg has less to do.
- A loop's induction value is an argument of the region (structured) or a
  block parameter (CFG), never a slot.
- mem2reg comes later as a pass, tested by the same oracle.

### Language constructs

- `vcall` stays until instruction selection, carrying the class and the
  method, so devirtualisation is a rewrite to `call` when the dynamic class
  is known. A destructor call on a local by value is already direct.
- Destructors and temporaries become **explicit calls** in the lowering,
  placed by a **cleanup stack** (clang's approach: each scope pushes what it
  must run; `return`, `break`, `continue` and the end of a block pop and emit
  them). This is what makes a single step from the tree to this IR feasible.

### Sketches (dump syntax, not final)

```
struct Point { x: f64, y: f64 }
layout Point (target haard64-be) { size 16, align 8, x @0, y @8 }

func @norm2(%p: ptr<Point>) -> f64 {
  %px = field %p, Point.x
  %x  = load %px
  %py = field %p, Point.y
  %y  = load %py
  %a  = fmul %x, %x
  %b  = fmul %y, %y
  %r  = fadd %a, %b
  ret %r
}
```

A counted loop in structured form, and the same loop vectorised:

```
func @saxpy(%a: f32, %x: ptr<f32>, %y: ptr<f32>, %n: i64) {
  for %i = 0 to %n step 1 {
    %xi = load (index %x, %i)
    %yi = load (index %y, %i)
    %m  = fmul %a, %xi
    %s  = fadd %m, %yi
    store %s, (index %y, %i)
  }
  ret
}

  %av = splat %a : vec<4 x f32>
  for %i = 0 to %nv step 4 { ... loads and stores of vec<4 x f32> ... }
  for %i = %nv to %n step 1 { ... the scalar body for the rest ... }
```

A virtual call and a destructor:

```
func @total(%s: ptr<Shape>) -> f64 {
  %name = slot String
  call @String.init(%name, @str.0)
  %r = vcall Shape.area(%s)
  call @String.destroy(%name)
  ret %r
}
```

The same loop after flatten, in CFG form:

```
@entry:          jmp @head(0)
@head(%i: i64):  %c = lt %i, %n
                 br %c, @body, @exit
@body:           ...
                 %i2 = add %i, 1
                 jmp @head(%i2)
@exit:           ret
```

### Tooling

- `IrPrinter` and `hdc --emit-ir`: read-only, for people and goldens.
- **Verifier** after the lowering and after every pass, always in the tests.
- **Source position per instruction**, from day one, and a mapping from
  machine instructions back to IR instructions.
- **`IrInterpreter`**: runs the CFG form to an exit status, with the natives
  of `std.low_io` inside it; endianness, layout and (for the processor) a
  timing table and a memory map are parameters. Every program the emitter
  suites run goes through both paths and must give the same status.

## 5. mem2reg in brief

A separate explanation with worked examples was written for Hadley outside
the repository. In short: for every slot whose address is used only by
direct loads and stores of its own type, delete the slot, replace each load
by the value last stored on that path, and put a merge (φ, or here a block
parameter) where different paths bring different values. Classic algorithm:
Cytron et al. 1991 (dominance frontiers, then renaming over the dominator
tree; dominators by Cooper, Harvey and Kennedy). Alternative that builds SSA
directly while lowering: Braun et al. 2013 (used by Cranelift's front end
and libFirm). A slot whose address escapes (a `T&` argument, a closure
capture, `self`, record 0068's temporaries) stays in memory until inlining
removes the call; Haard takes addresses often, so inlining before mem2reg
matters more than in C. Leaving SSA turns merges into copies at the end of
predecessors, which needs critical edges split and parallel copies ordered.

## 6. Alternatives considered

| Model | Used by | For | Against |
|---|---|---|---|
| Stack bytecode | JVM, WASM, CLR | trivial to produce, compact | poor to optimise; registers rebuilt anyway |
| Tree IR | Appel, GCC GENERIC | close to the source | no explicit flow; analyses need a graph |
| Sea of nodes | HotSpot C2, Graal | strong global optimisation | complex, hard to debug; V8 left it in 2023 |
| CFG + SSA, typed | LLVM | most documented, many targets | too low for SIMD/CGRA without heavy analyses, too abstract for a simple back end |
| Low-level, untyped temporaries | QBE | small, proven on amd64/arm64/rv64 | loop and array structure lost |
| Block parameters instead of φ | Cranelift, MLIR, Swift SIL | simpler merges | (a variant, adopted for the CFG form) |
| Two IRs | Rust MIR, Swift SIL | language and machine concerns apart | two of everything |
| Multi-level, structured | MLIR | loops kept for accelerators | a dependency, templates; ideas taken, code not |

## 7. Several targets

- QBE proves a low IR serves x86-64, ARM64 and RISC-V; one IR for all three
  of ours is not a risk.
- The IR is target independent; the **layout table** and the **machine IR**
  are per target. Endianness is a parameter of the layout and of the
  interpreter (the oracle against C++ runs in host order).
- **VLIW versus superscalar** touches only the machine IR and scheduling. A
  VLIW asks the IR for a `select` instruction (if-conversion) and for loop
  information kept down to the machine level (software pipelining, the same
  technique as CGRA mapping).
- **Fixed versus variable instruction length** touches only legalisation and
  the encoder (large constants split or pooled, branch relaxation). The IR
  only has to accept any 64-bit constant.
- **Dedicated registers** (the stack pointer reached by its own
  instructions) touch the machine IR, the prologue and epilogue and how
  locals are addressed. The IR never names the stack pointer as long as
  frames have a fixed size (no dynamic stack allocation); see the questions.

## 8. Real time: WCET on a PRET machine

### Where the analysis runs

Static WCET analysis has three steps:

1. **Flow analysis**: loop bounds, infeasible paths, targets of indirect
   calls. Easiest at **source and IR level** (`for i in 0...64` states its
   count); from a binary it often has to be annotated by hand.
2. **Microarchitectural analysis**: cycles per block (pipeline, caches,
   memory). Only possible on the **final machine code**.
3. **Path analysis**: the worst path, usually by IPET (an integer linear
   program over block execution counts).

The classic tools (aiT, OTAWA, Chronos) work on the binary, and their known
weakness is step 1: the compiler knew the bounds and threw them away.
WCET-aware compilers (WCC at TU Dortmund, platin for Patmos in T-CREST,
SWEET at Mälardalen) carry **flow facts** from the source down to the
machine code, updating them at every transformation.

A PRET machine (Edwards and Lee; PTARM, FlexPRET) makes step 2 simple: a
thread-interleaved pipeline with fixed latencies, scratchpads instead of
caches, a predictable memory controller, and timing instructions in the ISA
(`get_time`, `delay_until`, an exception when a deadline passes). What stays
hard is step 1, where the compiler knows the most.

**Proposal: hdc computes the WCET itself**, on the processor's final machine
IR, with flow facts from the structured form and a timing table of the
processor, and reports the worst path in source lines. An external tool can
be fed later if useful. x86-64 and ARM get no WCET.

### What it asks of the IR

- **Flow facts as first-class data**: every loop region carries a bound
  (inferred or annotated), and every pass that changes loops keeps it right
  (vectorising by 4 divides it). The verifier checks that every loop in
  real-time code has one.
- **Timing instructions and volatile accesses are barriers**: no
  optimisation moves code across them.
- **A mapping** machine code ↔ IR ↔ source.
- **Whole-program compilation**: the complete call graph, every indirect
  call's targets (the class tables give a virtual call's), the worst stack
  use.
- The interpreter gains a **cycle-counting mode** with the timing table: a
  second oracle, *measured cycles ≤ computed WCET* in every test.
- **Worst-case stack use** is computed by hdc: with no recursion and fixed
  frames it is a sum over the call graph, which also suits a dedicated stack
  pointer.

Language-level consequences (to be decided as records of their own): loop
bounds inferred or annotated, no recursion, no heap and only resolvable
indirect calls in real-time code; timing constructs as intrinsics.

## 9. Scratchpads and overlays

With scratchpads and no cache, **where** each piece of code and data lives
decides both whether the program runs and how long it takes. This is a
compiler job (WCET-directed scratchpad allocation is an established line of
work: Falk's WCC, Suhendra et al.), and it shapes the IR:

- **Memory regions belong to the target description**: a table of regions
  (base, size, read and write latency, code or data, private to a hardware
  thread or shared, reachable by DMA).
- **Every object that occupies memory is a named IR entity**: each
  function's code, each global, literal, constant table and vtable, and each
  stack. Instructions refer to objects **by symbol, never by address**.
  Placement is a **table** (object → region or overlay group) applied only at
  final layout, so it can be recomputed: analyse → place → analyse again,
  without redoing selection. This is the "as high level as possible" rule
  paying for itself.
- **Address spaces**: if the ISA reaches each memory with different
  instructions (Patmos has separate loads for its stack cache, local
  scratchpad, data cache and main memory), a pointer's type carries its
  address space (`ptr<T, spm0>`) and selection picks by it. If the same
  instructions reach every memory by address, the WCET still needs to know
  which latency a load pays; a points-to analysis over allocation sites gives
  each pointer its possible regions, and a pointer spanning regions pays the
  worst one.
- **Explicit transfers**: copying an object into a scratchpad (or starting
  and waiting for a DMA) is an IR instruction, a barrier for the optimiser
  and a cost the WCET sees.
- **Overlays**: functions and data never live at the same time (call graph
  and liveness) share a region; the compiler inserts the loads at entry
  points, statically. The interpreter keeps, for each overlaid region, which
  object is resident, and **stops the program** when one that is not is
  touched: an oracle for the overlay planner. A pointer into overlaid data
  that outlives the residency is the danger, so the first version overlays
  only code and data whose address does not escape.
- **Code size feeds back**: placement needs machine code sizes, so the
  processor's pipeline is IR → machine IR → sizes → placement and overlays →
  final layout → WCET, possibly iterated. **Function splitting** (moving a
  loop into a function of its own so that it fits) is an IR transformation,
  easy in the structured form.
- **The stack lives in a scratchpad** when its worst case is known, one per
  hardware thread.
- **Arrays larger than a scratchpad** can later be tiled with DMA double
  buffering, a transformation of the structured form, shared with the CGRA
  work.
- **Start-up**: something has to copy code into the instruction scratchpad
  before it runs; the compiler may have to emit that loader.

## 10. Storage: plain data by index, kept

Hadley allowed changing the model if it gets in the way. It does not, and
this design leans on it harder than before:

- **Side tables are keyed by id.** Flow facts, source positions, placement,
  the machine ↔ IR mapping and the analysis results are all parallel vectors
  indexed by an instruction, block, function or object id. With pointers
  they would be hash maps.
- **Fast and compact**: contiguous records, 4-byte indices, no allocation per
  instruction. The fastest recent compilers chose it on purpose (Cranelift's
  entity arenas with in-block order as an index-linked list, Zig's ZIR and
  AIR, Carbon's toolchain).
- **Rewritable in Haard** one to one, like the Ast and the tables.

Refinements for an IR that is edited heavily:

- **Storage per function**, not one global vector: cloning (inlining,
  splitting, unrolling), deleting and someday processing functions in
  parallel all work on one function's arrays.
- **Ids are stable inside a pass**: deletion marks; compaction runs between
  passes and returns a remap applied to every side table.
- **Replacing a value's uses** goes through an alias table (Cranelift's
  approach) or a rename map, with no use lists.
- The real cost is a bug, not speed: **never hold a pointer into a vector
  across an insertion** (the Ast already taught this).

Where efficiency will matter is the algorithms (dominators, worklists, no
quadratic walks), not the storage; the 500k-line measurement of 2026-09-09
found a linear scope walk, not the data layout.

## 11. Order of the work (once the questions are answered)

1. Types, tables (structs, classes, layout per target) and the target
   description.
2. The IR storage, `IrBuilder`, `IrPrinter` (`--emit-ir`), the verifier.
3. `IrInterpreter` on the CFG form: functions, calls, arithmetic, branches,
   memory by layout; then flatten.
4. The lowering from Ast + ResolutionTable, with the cleanup stack, one
   feature at a time, each compared against the C++ path.
5. Back ends (the processor, x86-64, ARM), the processor's placement and
   WCET.
6. Later: mem2reg, optimisations, vectorisation, CGRA kernels.

## References

- Cytron, Ferrante, Rosen, Wegman, Zadeck, *Efficiently Computing Static
  Single Assignment Form and the Control Dependence Graph*, 1991.
- Cooper, Harvey, Kennedy, *A Simple, Fast Dominance Algorithm*, 2001.
- Braun et al., *Simple and Efficient Construction of SSA Form*, CC 2013.
- Edwards, Lee, *The Case for the Precision Timed (PRET) Machine*, DAC 2007.
- Wilhelm et al., *The Worst-Case Execution-Time Problem: Overview of
  Methods and Survey of Tools*, 2008.
- Schoeberl et al., *T-CREST: Time-predictable Multi-Core Architecture for
  Embedded Systems*, 2015 (Patmos, platin).
- Falk, Lokuciejewski, *A compiler framework for the reduction of worst-case
  execution times* (WCC), 2010.
- QBE: c9x.me/compile; Cranelift; MLIR; WebAssembly's structured control
  flow.
