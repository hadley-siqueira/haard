# The IR questions, explained

The explanation given to Hadley for each question in
[ir-questions.md](ir-questions.md) before he answered it, and the answer he
chose. One section per question, added as each one is settled, so that the
reasoning behind every decision survives the conversation it was made in.
The surrounding design is [designing-the-ir.md](designing-the-ir.md).

## 1. Signed integer overflow

**The question.** `i32` holds −2,147,483,648 to 2,147,483,647. What is
`2147483647 + 1`? (a) it wraps to the minimum (two's complement); (b) the
program aborts; (c) it is undefined, as in C, and the optimiser may assume it
never happens.

No separate explanation was asked for. The argument given with the options:
C's undefined overflow lets an optimiser assume `i + 1 > i` and know a
loop's trip count, but a Haard `for` over a range already states its count
by structure, so (c) would buy little and cost every program a class of
silent bugs. Unsigned arithmetic always wraps.

**Answer: (a)**, overflow wraps.

## 2. Shift by a count ≥ the width

**The question.** `x << n` moves `x`'s bits `n` places. For `n` from 0 to 31
on an `i32` everyone agrees. What if `n` is 32 or more, or negative?

Arithmetic says shifting a 32-bit number by 32 pushes every bit out: the
result is 0. Hardware does not do that, because the shifter looks at only
some bits of the count:

| Processor | `x << 32` on a 32-bit value | Why |
|---|---|---|
| x86-64 | `x`, unchanged | only the count's low 5 bits: 32 & 31 = 0 |
| ARM64 | `x` | the count modulo 32 |
| RISC-V | `x` | the low 5 bits (6 for 64-bit) |
| ARM 32-bit | 0 | reads the whole low byte; ≥ 32 clears |
| POWER | 0 | reads 6 bits; 32 to 63 clear |

The hardware disagrees with itself, so C declared the case undefined.

**The real bug it causes.** A mask of the low `n` bits:

```haard
let mask = (1 << n) - 1     # n = 8 gives 0xFF; n = 32 gives ?
```

For `n = 32` the intent is all 32 bits set. With masking, `1 << 32` is
`1 << 0` = 1 and the mask is **0**, silently the opposite. With the
arithmetic rule, `1 << 32` is 0 and the mask is `0xFFFFFFFF` (overflow wraps,
answer 1). This shape appears in hashing, compression and peripheral bit
manipulation, which is everyday embedded code.

| Language | `x << 32` on 32 bits | Negative count |
|---|---|---|
| C, C++ | undefined | undefined |
| Java, C#, JavaScript | masked, gives `x` | masked too (−1 becomes 31) |
| Go | 0 (`>>` on a signed value fills with the sign) | aborts |
| Swift (`<<`) | 0 | shifts the other way |
| Rust | aborts in debug, masks in release | does not compile (unsigned count) |
| Zig | does not compile: a `u32`'s count must be a `u5` | same |

**Costs.** Only a count that is not a constant costs anything; `x << 3`, by
far the common case, is one instruction under every option, and a constant
count ≥ the width can be a compile error under every option. (a) Masking is
free on x86-64, ARM64 and RISC-V but gives the surprising mask above. (b)
The arithmetic rule costs about two instructions (a compare and a branchless
select) for a variable count, a constant time that does not disturb the
WCET. (c) Aborting adds a branch to every variable shift. On Hadley's
processor the shifter can implement the chosen rule directly.

The recommendation started as (a) for being free and was revised to (b)
once the mask example was weighed: the result is the arithmetic one,
coherent with answer 1, and it is the choice of the recent languages that
thought about it (Go, Swift).

**Answer: (b)**. `x << n` with `n` ≥ the width is 0; `x >> n` is 0, or the
sign fill for a signed `x`; a negative count counts as too large (same
results, never an abort); a constant count ≥ the width is a compile error.

## 3. Integer division by zero, and `MIN / -1`

**The question.** Two cases in which whole-number division has no valid
answer. It covers Haard's three division operators: `/` (truncates, as C),
`//` (floors, record 0057) and `%`. Floating point is out of it: IEEE 754
already says `1.0 / 0.0` is infinity and `0.0 / 0.0` is NaN.

- **Division by zero**: `10 / 0` has no mathematical answer, and the hardware
  has to do something.
- **`MIN / -1`, the only division that overflows**: an `i32` runs from
  −2,147,483,648 (`MIN`) to 2,147,483,647, and `MIN / -1` is +2,147,483,648,
  one past the maximum. No other whole-number division overflows. `MIN % -1`
  is 0, but some processors compute the remainder with the quotient and
  fail the same way.

| Processor | `x / 0` | `MIN / -1` |
|---|---|---|
| x86-64 | hardware exception (Linux kills the process with `SIGFPE`) | exception too, and `MIN % -1` as well |
| ARM64 | 0, no error | `MIN` (wraps) |
| RISC-V | quotient −1 (all ones), remainder `x` | `MIN`, remainder 0 |
| POWER | undefined (garbage), no error | undefined |
| MIPS | unpredictable; compilers add a check instruction | unpredictable |

| Language | `x / 0` | `MIN / -1` |
|---|---|---|
| C, C++ | undefined | undefined |
| Java | exception | `MIN` (wraps) |
| Go | aborts | `MIN` (wraps, in the specification) |
| C# | exception | exception |
| Rust | aborts, always | aborts, always, release included |
| Swift | aborts | aborts |

**The problem to solve.** On x86 the program dies with a useless message
("Floating point exception", for an integer). On ARM and RISC-V it **goes on
with a wrong value**. In a cyber-physical system, the processor's purpose,
that is the worst case: a controller divides by a zeroed sensor reading and
sends an actuator a command computed from 0 or −1, with no warning.

**Options.** (a) Abort with a message: a test `b == 0` before each division
whose divisor is not a constant (`x / 2` and `x % 10` pay nothing; a
constant zero is a compile error). In the WCET the abort path ends the
program and never joins the worst path; the cost is the comparison, constant.
Consistent with `unwrap` (record 0070). (b) Undefined: free, but the
behaviour varies by processor, silent wrong values included, against the
spirit of answers 1 and 2. (c) A defined value such as RISC-V's: free on
RISC-V, a check on x86 and ARM, and still a silent wrong value.

With answer 1, the two cases separate naturally: if overflow wraps, the
coherent answer is `MIN / -1` = `MIN` (the same as `-MIN`) and
`MIN % -1` = 0, as Java and Go do and as ARM64 and RISC-V give for free;
only x86 needs an extra check for a divisor of −1 on a signed, non-constant
division. On Hadley's processor the zero check can be free if the divide
instruction traps to the abort routine (question 25).

**Answer: (a)**, with `MIN / -1` giving `MIN`. A zero divisor aborts with a
message; a constant zero divisor is a compile error; `MIN / -1` wraps to
`MIN` and `MIN % -1` is 0.

## 4. Exceptions with unwinding

**The question.** Whether Haard will ever have `throw` / `try` / `catch`.

```cpp
double read_temperature() {
    if (!sensor_ok()) throw SensorError();   // leaves from here...
    return read();
}

void control() {
    Buffer b(1024);                  // has a destructor
    double t = read_temperature();
}

int main() {
    try { control(); }
    catch (SensorError& e) { safe_mode(); }  // ...and lands here, two calls up
}
```

A `throw` jumps back through the call stack to the nearest `catch`, through
as many functions as it takes, and each function crossed must run its
locals' destructors (`b` is freed though `control` never finished). That
walk is **unwinding**.

Haard today has none: expected errors are values (`Option`, enums) that the
caller checks; errors that should not happen (`unwrap` of `None`, division
by zero) abort with a message.

**How exceptions are implemented.**

1. **Tables** (modern C++'s "zero cost"): nothing extra on the normal path,
   but the compiler emits per-function tables of destructors and handlers,
   and a `throw` runs a run-time unwinder (`libunwind` / `libgcc`) that
   reads them and walks frame by frame. A throw costs thousands of cycles,
   depending on depth and destructors.
2. **`setjmp` / `longjmp`**: every `try` saves registers, paid even without
   an error. Old, little used.
3. **Errors as return values** (Rust, Zig, Go, Swift underneath): no
   unwinding; the function returns "result or error" and the caller tests
   with an ordinary branch. Rust's `?` and Zig's `try` propagate with one
   token; Swift's `throws` looks like exceptions but returns the error in a
   register.

**What (b) would do to the IR.** Any call might not return normally. In
LLVM, every call that may throw becomes an `invoke` with two destinations,
normal and unwind, and the unwind one starts with a `landingpad` that runs
the scope's destructors and propagates. The CFG nearly doubles its edges and
every optimisation, the verifier and mem2reg must respect them; every scope
with a destructor gets two cleanup paths; the interpreter must unwind; every
back end must emit tables in its platform's format and every target needs an
unwinder.

**What it would do to real time.** A throw's time depends on stack depth,
tables and the handler search, hard to bound tightly and far worse than a
branch. Safety-critical standards say so: JSF AV C++ (F-35 avionics) forbids
exceptions, and much embedded C++ is built with `-fno-exceptions`. Tables
and the unwinder also take scratchpad space.

**Why decide now.** Adding exceptions later is among the most invasive
changes a language can make: all existing code can then be interrupted at
any call and must stay correct (exception safety), and the IR, back ends and
optimisations all need revisiting. Deciding "never" closes nothing useful:
recoverable errors with propagation sugar (a `Result<T, E>` with something
like Rust's `?`) are ordinary values and branches and need nothing from the
IR.

**Answer: (a)**, never. The IR has no `invoke`, no landing pads, no unwind
tables and one cleanup path per scope. What "abort" means on the processor
(a safe-mode routine rather than a halt?) is question 29.

## 5. Aliasing between pointers and `T&`

**The question.** Aliasing is two names reaching the same memory:

```haard
def add_twice : void
    @a : i32&
    @b : i32&
    a = a + b
    a = a + b

let x = 1
let y = 10
add_twice(x, y)     # different things: x = 1 + 10 + 10 = 21
add_twice(x, x)     # the SAME variable: x = 1 + 1 = 2, then 2 + 2 = 4
```

**Why it matters.** An optimiser keeps values in registers and reorders or
groups memory accesses, which it may only do when a write cannot change what
another read sees. In `add_twice`, without overlap `b` is read once and the
result is `a + 2*b`; with possible overlap `b` is re-read after every write
to `a`, multiplied by every loop iteration. In the `saxpy` loop
(`y[i] = a * x[i] + y[i]`), vectorising reads `x[0..3]` and `y[0..3]` at
once; if `y` points at `x + 1`, the write to `y[0]` *is* `x[1]`, which the
scalar loop would read already changed, so the result differs and
vectorising is forbidden. Copying an array into a scratchpad or the CGRA is
the same problem, and knowing where a pointer may point tells the WCET which
memory, and so which latency, an access pays.

| Language | Rule |
|---|---|
| C | may always overlap; `restrict` (C99) is the programmer's promise, false = undefined; little used, easy to get wrong |
| C++ | may overlap; `__restrict__` only as a compiler extension |
| Fortran | arguments may not overlap **if one of them is modified** during the call; the historical reason Fortran beat C at numerics |
| Rust | `&mut` is exclusive, enforced by the borrow checker |
| Zig | may overlap; `noalias` on a parameter is a promise |
| Java, Go | may always overlap |

**Options.** (a) Always may overlap: the program always does what it says;
a vectoriser emits two versions of a loop and a run-time overlap test, which
costs the test, duplicated code (scratchpad space) and a WCET that counts
the slower version. (b) `T&` parameters never overlap, by the programmer's
promise. (c) (a) plus an opt-in annotation, like `restrict`, checkable at
run time in a debug mode.

**A correction made during the discussion.** Hadley asked how Fortran
manages `matrix.mul(m, m)` if arguments never overlap. Fortran's rule only
forbids overlap when one side is **written**:

```fortran
call mul(m, m, r)     ! legal: m only read twice, the result goes to r
call mul(m, m, m)     ! illegal: m read by the first two and written by the third
r = matmul(m, m)      ! legal: matmul is a function returning a new array
m = matmul(m, m)      ! legal: the right side is evaluated before the assignment
```

The first explanation of (b) had overstated it, claiming `v.dot(v)`, `a == a`
and `mul(m, m)` would become undefined. They would not, and the correction
also showed that **aliasing only matters when there is a write**: a function
that only reads through its parameters is optimised freely, overlap or not,
so (a) loses only in functions that write through one reference and read
through another, which are exactly those where overlap changes the result.

What (b), even with Fortran's exact rule, would still make undefined are
written overlaps, common in Haard because `T&` is also the don't-copy
convention (Haard has no `const`, record 0029):

```haard
xs.push(xs[0])        # push takes the element by T& and writes xs (self)
```

`push` modifies `self` and may reallocate the buffer the element lives in.
C++ requires `vector::push_back(v[0])` to work and implementations take
special care; Rust refuses it at compile time. `list.append_all(list)`,
`m.mul_in_place(m)` and `add_twice(x, x)` have the same shape. Under (b)
they would work in debug and could go wrong only when optimised.

The compiler also proves non-overlap itself in many cases: two different
local arrays, two globals, two results of `new`, and two distinct `Array`
objects, which own their buffers (copying an `Array` copies the buffer,
record 0031). Overlap only appears when one object arrives by two paths, as
in `f(xs, xs)`, or through raw pointers.

**Answer: (a)**. Any two pointers or references may overlap, and there is
no annotation: Hadley does not want a new keyword for it. Where the compiler
cannot prove non-overlap, a vectoriser or a scratchpad/CGRA transfer uses a
run-time test with a scalar fallback.

## 6. Type-based aliasing ("strict aliasing")

**The question.** Question 5 asked *whether* two pointers may reach the
same memory. This one asks about C's extra rule: **two pointers of different
types never overlap**. An `f32*` and an `i32*` never point at the same
place, and the compiler may rely on it. The rule is *strict aliasing*; the
optimisation built on it is **TBAA** (type-based alias analysis). In C an
object may be read only through its own type (or its signed/unsigned
variant, or `char` for raw bytes); reading a `float` through an `int*` is
undefined.

**What it lets the compiler do.**

```haard
class Signal:
    data : f32*
    size : i32

    def scale : void
        let i = 0
        while i < size:                 # self.size, an i32
            data[i] = data[i] * 2.0     # writes an f32
            i = i + 1
```

Each iteration writes an `f32` and then reads `self.size`. Without the rule
(and with answer 5), `data[i]` might be `size` itself, so `size` is re-read
every iteration, the trip count is unknown and the loop cannot be vectorised
without a run-time test. With the rule, writing an `f32` cannot change an
`i32`: `size` is read once and the loop vectorises.

**What it costs.** Everything that reads one piece of memory as two types
becomes undefined, which is common embedded code:

```haard
let x : f32 = 1.5
let p = &x as u32*
let bits = *p                 # a float's bits: undefined under the rule

let packet : u8* = receive()
let head = packet as Header*
let kind = head.kind          # a byte buffer read as a struct: undefined
```

C allows the first only through a `union` or `memcpy`; C++ not even through
a union. The Linux kernel is built with `-fno-strict-aliasing` because
system code does this constantly (protocols, drivers, device registers), and
the failures appear only with optimisation on, and only sometimes.

| Language | Rule |
|---|---|
| C, C++ | exists (except `char`); many projects turn it off |
| Rust | none; optimisation comes from borrowing (`&mut` is exclusive) |
| Zig, Go | none |
| Java, C# | does not apply: memory cannot be read as another type |

**Haard's case.** Haard allows exactly what the rule forbids: `union` is C's
(record 0064), `as` converts pointer to pointer (record 0049), and the
processor targets cyber-physical systems where reading buffers as structs and
touching device registers is routine. The gain would also be smaller than in
C: a `for` over a range evaluates its bound once (checked in the emitted
C++: `for i in 0...*n` copies `*n` before the loop), so `for i in 0...size`
has no problem; and whole-program analysis proves much of the rest (a buffer
made by `new` inside the class that never escaped cannot be the `size`
field). What remains are `while` loops that re-read a field.

**Answer: (a)**, no type-based rule. Any pointer may reach anything of any
type; reinterpreting memory is always legitimate; no load or store in the IR
carries type information for aliasing. Coherent with answer 5.

## 7. Floating point: strict or relaxed

**The question.** Mathematically `(a + b) + c` equals `a + (b + c)`. In
floating point it does not, because every operation rounds:

```haard
let a = (0.1 + 0.2) + 0.3     # 0.6000000000000001
let b = 0.1 + (0.2 + 0.3)     # 0.6

let x = (1e20 + -1e20) + 1.0  # 1.0
let y = 1e20 + (-1e20 + 1.0)  # 0.0, the 1.0 is lost against -1e20
```

**Strict** means the compiler computes exactly in the order and form
written, as IEEE 754 says; **relaxed** means it may rearrange as if the
numbers were real, for speed.

**Why a compiler wants to rearrange.**

1. **Summing an array with SIMD.** `s = s + x[i]` in a loop is written as
   `((((s + x0) + x1) + x2) + x3)...`, each sum waiting for the previous.
   Vectorising keeps four partial sums and combines them at the end: another
   order, so another result in the last bits. Strict mode cannot vectorise
   it automatically, nor map it onto the CGRA as an adder tree.
   Element-by-element operations (`saxpy`) do not change order and
   vectorise in strict mode too; only **reductions** (sums, products,
   maxima) are affected.
2. **FMA** (fused multiply-add): `a*b + c` in one instruction with a single
   rounding, faster and more precise, but **different** from a separate
   multiply and add. Fusing automatically is *contraction*.
3. **Algebraic simplifications** that look obvious and are false: `x * 0.0`
   to `0.0` (false for infinity and NaN), `x - x` to `0.0` (same),
   `x / 10.0` to `x * 0.1` (0.1 is not exact). GCC's `-ffast-math` also
   assumes NaN and infinity never occur, which can delete an `is_nan(x)`
   test.

**Why it matters here.** *Reproducibility across targets*: a common
cyber-physical workflow develops and validates a controller on a PC (x86)
and then runs it on the embedded processor. If one target contracts into FMA
and another does not, or they reorder differently, the same program gives
different numbers and the validation on the PC no longer holds for the
board. It happens in practice: GCC contracts by default when the processor
has FMA, so x86 with and without FMA and ARM can diverge. *The oracle*:
testing compares the C++ path's exit status with the IR interpreter's; if
one side reorders or contracts and the other does not, floating-point
programs diverge with no bug anywhere. In strict mode both must agree bit
for bit, which also requires the emitted C++ be built with
`-ffp-contract=off`.

| Language | Default | How to relax |
|---|---|---|
| C, C++ | strict, but GCC contracts FMA by default | `-ffast-math`, for a whole file |
| Rust | strict always | no stable way; `mul_add()` for explicit FMA |
| Java | strict always (since Java 17) | `Math.fma()` |
| Julia | strict | `@fastmath` or `@simd` on a block or loop |
| Zig | strict | `@setFloatMode(.optimized)` in a scope |
| Fortran | may reorder what is mathematically equivalent, **except across written parentheses** | — |

**Options, with no new keyword (as Hadley asked in question 5).** (a) Strict
always: nothing reordered or contracted; FMA exists as a library function
`fma(a, b, c)`, exact and the same on every target; a vectorised reduction
is written by hand with the vector types of question 47, in an order the
programmer chose; automatic vectorisation still covers everything
element-wise. (b) Strict by default with a way to relax (a per-module option
in the manifest, or library functions with unspecified order such as
`xs.sum_unordered()`), whose results stop matching across targets where
relaxed. (c) Relaxed by default: faster, results vary across targets and
compiler versions, and the oracle suffers.

The first list recommended (b); after the reproducibility argument the
recommendation became (a), with (b) addable later without undoing anything.

**Answer: (a)**, strict always. Floating-point operations are never
reordered or fused by the optimiser; `fma` is an IR instruction emitted only
when written.

## 8. Rounding mode

**The question.** Every floating-point operation produces an exact result
that rarely fits the available bits, so it must be **rounded**. IEEE 754
defines several ways:

| Mode | `1/3` to 4 decimal digits, for illustration | Note |
|---|---|---|
| **To nearest, ties to even** | 0.3333 | the **default** everywhere |
| Toward zero (truncate) | 0.3333 | |
| Toward +∞ (up) | 0.3334 | |
| Toward −∞ (down) | 0.3333 | |

(For `-1/3`, up gives −0.3333 and down −0.3334.)

**How hardware does it.** The current mode lives in a control register
(`MXCSR` on x86, `FPCR` on ARM, `fcsr` on RISC-V) that every floating-point
operation consults. A C program changes it at run time with
`fesetround(FE_DOWNWARD)`, and from then on **every** computation, libraries
included, rounds down until someone changes it back. RISC-V also lets each
floating-point instruction **fix the mode in the instruction itself**,
independent of the register; AVX-512 allows it on some instructions.

**Why anyone changes it.** *Interval arithmetic*: to know for certain where
a computation's true value lies, compute it once rounding down (a lower
bound) and once rounding up (an upper bound); the interval is guaranteed to
contain the exact answer. It has real use in critical software and formal
verification, such as proving a controller never exceeds a limit despite
rounding errors. *Testing numerical sensitivity*: run the same computation in
several modes and see how much the result moves. Apart from that, almost all
code runs in the default mode all its life.

**The problem with allowing it (option b).** If the mode is global state
that changes at run time, every floating-point operation depends on a hidden
value:

- The compiler cannot fold constants: `0.1 + 0.2` written in the source
  depends on the mode in force when the line runs.
- It cannot move floating-point operations across calls, since any function
  may have changed the mode; every operation gains a side effect. LLVM needed
  a whole family of special instructions for this (the constrained
  floating-point intrinsics), and C needs `#pragma STDC FENV_ACCESS ON`,
  which GCC does not fully implement.
- Reproducibility (answer 7): a library that changes the mode and does not
  restore it silently alters the rest of the program's results.
- Hardware threads and interrupts must save and restore the control
  register, one per thread on the processor.
- The C++ path: `g++` honours mode changes only with `-frounding-math`,
  which turns optimisations off.

**Options.** (a) Fixed, always to nearest (ties to even): every
floating-point operation is a pure function of its operands; the compiler
folds constants, moves and eliminates repeated computations freely (within
answer 7's order); the result is the same at compile time and run time, and
on every target. (b) Changeable by the program, as C's `fesetround`, with all
the costs above.

**A third way, for later.** Interval arithmetic needs no global state:
functions that fix the mode on one operation, such as `add_down(a, b)` and
`add_up(a, b)`, are an attribute of the IR instruction and keep it pure; free
on RISC-V (and perhaps on the processor) through the instruction's mode
field; on x86 and ARM the back end switches the control register around the
operation; no keyword, just library functions. Addable later without undoing
anything.

The recommendation was (a), coherent with answer 7, with a suggestion for
the processor: a **rounding-mode field in each floating-point instruction**,
as RISC-V has, keeps the third way open for free (question 27).

**Answer: (a)**, fixed, round to nearest, ties to even.

## 9. Array bounds checks

**The question.** Whether every indexed access checks that the index is
inside the array:

```haard
let xs = [10, 20, 30, 40, 50]    # 5 elements, indices 0 to 4
let v = xs[7]                     # index 7 does not exist
xs[7] = 99                        # writing where nothing is
```

**What happens today, unchecked.** `Array`'s `operator[]` in
`std/array.hd` is just `return data[i]`. Reading out of bounds returns
whatever memory follows (another variable, garbage): a **silent wrong
value**. Writing out of bounds overwrites another variable, another object's
field or a function's return address: the program may break much later, far
from the mistake, or keep running on corrupted data. This is the **buffer
overflow**: Microsoft and Google report about **70%** of their serious
security bugs are memory errors of this kind, and Heartbleed (OpenSSL, 2014)
was an out-of-bounds read. In a cyber-physical system the equivalent is a
wrong index silently changing the neighbouring variable that drives an
actuator.

**Where a check is possible.** Only where the length is known: fixed arrays
(`i32[4]`, in the type) and `Array`, `List`, `String` (a field). A raw
pointer (`T*`, including record 0028's `new T[n]`) does not know its length,
so indexing one is never checked under any option. That is also the natural
unchecked way out for whoever wants it (`xs.data[i]`), with no new keyword.

**The cost.** A compare and a branch per access (`if i >= size: abort`; one
**unsigned** compare covers negative and too-large indices). The optimiser
removes most of them:

```haard
for i in 0...xs.size():
    total = total + xs[i]     # i < xs.size() is guaranteed by the for itself
```

The compiler **proves** `i` is in bounds and drops the check (bounds-check
elimination), easy in Haard because a `for` over a range states its limits by
structure. Where the proof fails, the check can be **hoisted** before the
loop (one test, `n <= xs.size()`, instead of one per iteration), which also
keeps the loop vectorisable. Rust, Go, Java and Swift check always and
measure typical costs of 0 to 5%. In the WCET each check is a fixed compare;
the abort path ends the program and never joins the worst path.

| Language | Rule |
|---|---|
| C | never checks |
| C++ | `v[i]` unchecked; `v.at(i)` checked (opt-in, little used) |
| Rust, Go, Java, C#, Swift | **always** check (Rust's `get_unchecked`, Swift's `-Ounchecked` to opt out) |
| Zig | checked in Debug and ReleaseSafe, not in ReleaseFast and ReleaseSmall |
| **Ada** | **always checks** by default, and is *the* language of critical real-time systems (aviation, rail, defence); SPARK, its subset, **proves** at compile time that no access leaves its bounds |

**Options.** (a) Never, as C: fast, and every index error is silent
corruption. (b) Always, aborting with a message, as Rust, Go, Ada; the
optimiser removes what it can prove; the raw pointer is the way out. (c)
Only in a debug mode, as Zig's ReleaseFast: tests catch errors, but the
program **in the field** behaves differently from the one tested.

**The recommendation was revised from (c) to (b).** Coherence with answer
3, where division by zero aborts rather than giving a wrong value: an index
out of bounds is the same kind of error. The weakness of (c) is exactly the
case that matters: in a cyber-physical system what runs is the production
program, so a checking debug build and an unchecked production build test
one program and ship another. The cost is small and analysable. Record 0047
is not contradicted: it says the compiler never *refuses* a program out of
disapproval, and a run-time check refuses nothing; it stops an error from
becoming corruption, and the raw pointer remains for whoever opts out. And
Ada, the standard language of critical real-time work, always checks.

**In the implementation.** Fixed arrays: the compiler inserts the check.
`Array`, `List`, `String`: the check lives in the library's Haard
`operator[]` (record 0034), calling the abort (record 0070). In the IR the
check is a dedicated high-level instruction, such as `check_index %i,
%size`, not an ordinary `if`, so the optimiser recognises it to eliminate or
hoist it and the WCET recognises a path that does not return; it becomes a
compare and branch only at instruction selection. The library reaches it
through an intrinsic.

**Answer: (b)**, always checked, aborting with a message.
