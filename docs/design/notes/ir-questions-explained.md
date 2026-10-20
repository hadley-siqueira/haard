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

## 10. Reading uninitialised memory

**Where it happens in Haard.** Named locals are protected: the compiler
already refuses a use before the `let` (record 0063). The problem is memory
that exists without having been written: class fields `init` does not write
(record 0026 leaves fields without a starting value unless written), the
buffer of `new T[n]` (record 0028), and the spare capacity of an `Array` or
`List` past its `size`.

```haard
class Sensor:
    reading : f32
    count : i32

    def init : void
        reading = 0.0          # count is NOT written

let s = Sensor()
let c = s.count                # what is c?
```

**What C does, and why it is dangerous.** In C, reading uninitialised memory
is in several cases **undefined behaviour**. Inside LLVM it becomes the
special values `undef` and `poison`, with surprising rules: an `undef` **may
be a different value each time it is read** (`x == x` need not be true), and
a branch on a `poison` makes the whole program undefined, so the optimiser
may **delete whole paths**. A documented C++ case: an uninitialised `bool`
can make **both** branches below print, or neither:

```cpp
bool b;                        // uninitialised
if (b)  puts("true");
if (!b) puts("false");
```

The optimiser is not wrong: the case is undefined, so anything is allowed.
But the program then behaves in a way no reading of its source explains.

**Where the field is going.** C++26 changes reading an uninitialised
variable from undefined to **erroneous behaviour**: some value, valid, with
no cascade, which is option (a). GCC and Clang offer
`-ftrivial-auto-var-init=zero` (or `pattern`), used by the Linux kernel,
Android and Windows, so uninitialised memory is never unpredictable garbage.
Rust forbids the read (it needs `MaybeUninit`); Go and Java zero all memory.

**Options.** (a) An unspecified value with no other effect: a read gives
some bit pattern, whatever is there; reading again gives **the same** value
until a write; nothing else happens, no path is deleted, `x == x` is always
true. LLVM calls this freezing the value (the `freeze` instruction). Haard's
IR would have no `undef` and no `poison`; what (b) would allow on top is rare
and contrived. (b) Undefined, as in C: the optimiser may assume it never
happens, and the IR needs `undef` / `poison` with propagation rules, one of
the subtlest and most bug-prone parts of LLVM itself.

A third way outside the list, **zeroing everything** (Go, Java), was already
rejected by record 0026 (fields have no starting value unless written), and
would cost time proportional to every allocation (zeroing a
`new f32[10000]`), time the WCET would have to count.

**What (a) means in practice.** No WCET impact: a read is a read. In
mem2reg, a slot read before any write can become a fixed constant such as 0,
since "some value" includes 0, deterministically. A program whose result
depends on uninitialised memory may give different exit statuses in the
interpreter and the C++ path; that is a bug in the program, and to catch it
the interpreter can have a mode that **fills all new memory with a garbage
pattern** (such as `0xAA`), like the upper-bits mode discussed for the
untyped IR. The C++ path: `g++` still treats part of this as undefined;
building with `-ftrivial-auto-var-init=pattern` brings it close to (a).

The recommendation was (a): coherent with every answer so far (1a, 2b, 3a,
6a, 7a, 9b) -- **no case in Haard is "undefined"**; every error either has a
defined result or aborts with a message. The IR loses `undef` / `poison`, a
whole class of complexity and optimiser bugs, and C++26 is heading the same
way.

**Answer: (a)**, an unspecified value with no other effect.

## 11. Reaching device registers (`volatile`)

**The question.** In embedded systems, devices (UART, timers, GPIO, the
CGRA's configuration, the DMA controller) appear as **memory addresses**.
Reading or writing one is not storing data: it is **talking to the
hardware**.

```haard
let uart_status = 0x1000_0004 as u32*
let uart_tx     = 0x1000_0000 as u32*

while (*uart_status & 1) == 0:   # wait until the device is ready
    pass
*uart_tx = 72                    # send 'H'
*uart_tx = 105                   # send 'i'
```

This is **MMIO** (memory-mapped I/O), and the optimiser, treating these
addresses as ordinary memory, breaks it:

| Optimisation | On ordinary memory | On a device register |
|---|---|---|
| Read the same address once, outside the loop | right: nobody changed it | the loop becomes **infinite**: the hardware changes the status and the program never re-reads |
| Drop the first of two writes to one address | right: the second overwrites | the `'H'` is **never sent** |
| Reorder independent accesses | right | "configure" and "start" swap: the device starts with the old configuration |
| Merge two 16-bit writes into one 32-bit, or read 32 bits as 4 bytes | right | the device needs the **exact width**; another width is another operation, or an error |

`volatile` tells the compiler that **this access happens exactly as
written**: every read and write, in the written order and width, nothing
eliminated, merged or repeated. Two common misconceptions: `volatile` is
**not** atomic and is **not** for communication between threads; it exists
for hardware.

**Where it matters on the processor.** Drivers (a UART to print, timers,
GPIO; the processor's `std.low_io` natives, question 29, will likely be
written this way). The CGRA and DMA: write the configuration, start, wait for
"done", where the **order relative to ordinary memory** also matters: after
starting a DMA copy, the program must not read the buffer before "done", and
the compiler must not move that read earlier. That is why proposal 85 makes
volatile accesses **full barriers** in the IR (no ordinary access crosses
one), stronger and safer than C, which only orders volatiles among
themselves.

| Language | Form |
|---|---|
| C, C++ | a **type qualifier**, `volatile uint32_t* p`; C++20 **deprecated** several uses (`v += 1` on a volatile, volatile parameters) because the semantics confused people |
| Zig | a pointer qualifier, `*volatile u32` |
| Rust | **no volatile type**: functions `read_volatile(p)` and `write_volatile(p, v)`; embedded libraries generate register classes on top |
| Ada | an aspect on the object (`Volatile`), with clauses fixing its address |
| Linux kernel (in C) | despite `volatile`, uses **functions** `readl()` / `writel()` for registers, for clarity and control of barriers |

**Options.** (a) A `volatile` qualifier on the pointer type, as C and Zig
(`0x1000_0000 as volatile u32*`, then `*uart = 72`): natural syntax and lets
a struct of registers be overlaid, but a new keyword, and it reaches the
whole type system (does `volatile u32*` convert to `u32*`? overload
resolution? generics?), which C++ spent decades getting right and still
deprecated parts of. (b) Intrinsic functions in the library, as Rust and the
Linux kernel (`volatile_write(uart, 72)`,
`while (volatile_read(status) & 1) == 0`): no new keyword and no type-system
change -- ordinary generic functions in `std` that the compiler recognises
and turns into volatile loads and stores; every special access visible where
it is made; a `Register` class with `read()` / `write()` can be built on top;
`T` limited to 8-, 16-, 32- and 64-bit whole numbers since the width must be
exact; a `fence()` in the same family covers ordering without a volatile
access. Against it: more verbose, and nothing stops an ordinary `*p` on a
device address by mistake; the type does not protect. (c) Only the compiler
generates them: drivers would be natives written inside the compiler, as
`std.low_io`'s are today, so every new driver would need a compiler change;
not viable for a language meant to run bare metal on an embedded processor.

Hadley had noted, before this question, that Haard has no `volatile` and
must have it. Both (a) and (b) meet that; only (a) needs a keyword, and he
had declined a keyword for an annotation in question 5.

The recommendation was (b): no keyword, no type-system complexity, the
choice of Rust and the Linux kernel even where an alternative existed, and
additive -- a qualifier can come later without undoing the functions.

**Answer: (b)**: `volatile_read`, `volatile_write` and `fence` in the
library, emitted as volatile loads and stores that are barriers in the IR.

## 12. New numeric types

**The question.** Haard has `i8`…`i64`, `u8`…`u64`, `isize` / `usize`
(record 0050), `f32` and `f64`. Should any of these join them?

**`f16`, IEEE 754 half precision.** 5 exponent bits and 10 mantissa bits
(against `f32`'s 8 and 23); largest value 65,504; about 3 decimal digits.
Half the memory and **twice the values per SIMD register**: used in neural
network inference, graphics, signal processing and sensor data, where 3
digits suffice and memory bandwidth is the bottleneck. Hardware: ARMv8.2
(phones) computes in it; x86 long only converted (F16C) and computes only
with AVX512-FP16 (Sapphire Rapids); RISC-V has the Zfh extension. Without
hardware the compiler converts to `f32`, computes and rounds back; for the
four basic operations that gives **exactly** the correctly rounded IEEE
result, because `f32` has bits to spare, so emulation does not break
answer 7.

**`bf16`, Google's "brain float".** 8 exponent bits and 7 mantissa bits: an
`f32` with its mantissa cut, the **same range** as `f32` with 2 to 3 digits
of precision; converting to `f32` is a 16-bit shift. Used to train neural
networks, where range matters more than precision; many accelerators work in
it (perhaps a CGRA too). Hardware: x86 (AVX512-BF16, AMX), ARMv8.6, TPUs,
GPUs. **Not an IEEE standard**: rounding and denormals vary between
implementations, which conflicts with answer 7 (the same result on every
target) unless Haard fixes the rules itself.

**`i128` / `u128`.** Uses: the **full product of two `u64`** (64 × 64 =
128 bits) in fixed-point arithmetic, cryptography, hashing and overflow
detection; 128-bit identifiers (UUIDs); huge counters. No target has 128-bit
integer registers: each operation becomes a sequence of 64-bit ones (add with
carry, the high half of a multiply) and division a library routine. Rust and
Zig have them; C has `__int128` as a GCC/Clang extension.

**What adding a type costs.** It crosses the whole compiler: a new built-in
type name (as `isize` was, record 0050); the type of literals (what is `1.5`
when the destination is `f16`?); `as`'s conversion list (record 0049); the
C++ mapping (`std::float16_t`, `std::bfloat16_t` from C++23, `__int128`);
in the IR the type exists and **each back end legalises it** into operations
its processor has (`f16` without hardware becomes conversions to `f32`,
`i128` pairs of 64-bit operations); in the WCET the legalisation sequences
have a fixed length and are analysable, 128-bit division included (a loop of
128 iterations).

**The main point: it is additive.** Adding a numeric type later breaks
nothing and changes the meaning of no program. All that must be decided now
is that the IR's type representation be open to new widths and formats
(integers of any width, floating-point formats as an extensible list), which
is done in any case.

**An alternative for the commonest 128-bit use.** The full product of two
`u64` can be a library function returning a tuple, which Haard already has:

```haard
let (high, low) = mul_wide(a, b)    # a, b : u64
```

Every target has an instruction for the high half of a multiply, so it costs
two instructions.

**Options.** (a) None for now, the IR ready for them, `mul_wide` covering the
main 128-bit use; (b) `f16` now; (c) `bf16` now; (d) `i128` / `u128` now;
combinations allowed.

The recommendation was (a): `f16` and `bf16` pay only with SIMD and the CGRA
(twice the values per register, accelerators working in those formats), so
they are better decided with questions 46–48 and 51, knowing what the SIMD
and CGRA support, and `bf16` needs rounding rules of its own to respect
answer 7; `i128`'s main use is covered by `mul_wide`; nothing closes a door.

**Answer: (a)**, none for now; `mul_wide` in the library.

## 13. Dynamic stack allocation

**The question.** Reserving space **on the stack** with a size known only
**at run time**:

```c
void process(int n) {
    float buf[n];                              // C99 variable-length array (VLA)
    float* tmp = alloca(n * sizeof(float));    // the same idea, as a function
}                                              // both vanish when the function returns
```

In Haard today, fixed arrays (`i32[4]`) have a compile-time size and live on
the stack; `new T[n]` (record 0028) has a variable size but lives on the
**heap** and needs `delete`; nothing has a variable size on the stack. It was
checked that no current Haard construct needs one: every type has a fixed
size, tuples and closure environments (record 0058) included.

**Why anyone wants it.** It is fast (reserving is moving the stack pointer,
no allocator call), needs no freeing (gone at function exit), does not
fragment memory like the heap, and seems the natural escape from a heap ban
in real-time code (question 60 recommends forbidding `new` there).

**What it costs.**

1. **Worst-case stack use stops being computable.** With fixed frames and no
   recursion the compiler computes the exact maximum stack a program can
   use, a sum over the call graph. With `buf[n]` the maximum depends on `n`.
   On the processor this weighs twice: the stack lives in a **scratchpad**
   (question 41) that needs a known size to be reserved, and without an MMU
   and perhaps without overflow detection (question 21), **overflowing the
   stack silently corrupts the neighbouring memory**.
2. **A known source of serious failures.** The **Linux kernel removed every
   VLA** in 2018 (Linus Torvalds called them a bad idea) after the *Stack
   Clash* family of vulnerabilities, where a large `n` jumps the stack's
   guard page and writes into another region. **MISRA C** (automotive) and
   **JSF AV C++** forbid VLAs; **C11** made them optional; **C++ never
   accepted them** (only as a GCC extension).
3. **It complicates code generation and the IR.** If the stack pointer moves
   by an unknown amount mid-function, locals can no longer be addressed from
   it: every function using it needs a fixed **frame pointer**, which weighs
   with the processor's dedicated SP (questions 16–18). The IR would gain a
   dynamic allocation instruction plus save/restore of the SP for VLAs inside
   loops (LLVM's `stacksave` / `stackrestore`), so **the IR would start
   touching the stack pointer**, today hidden in the back end.

| Language | Rule |
|---|---|
| C | VLAs and `alloca` exist; optional since C11 |
| C++ | none (extension only) |
| Rust | none; proposals postponed for years |
| Zig | none, **on purpose**: the proposal was declined to keep stack use computable |
| Go | the compiler picks stack or heap by escape analysis, but stacks grow dynamically (unfit for real time) |
| Ada | allows dynamically sized arrays on the stack; real-time profiles and SPARK restrict them |

**Variable-size buffers, then?** The safety-critical norm is the **maximum
size**: a fixed buffer with the largest capacity the case needs, and a count
of how much is in use; a library class can package it. For the rare cases
where even that fails, a time-bounded **pool allocator** (option (b) of
question 60) serves without touching the stack.

**Options.** (a) It does not exist: every frame has a compile-time size; the
compiler computes each thread's worst stack use; the IR never mentions the
stack pointer (every slot static, the back end sizes the frame); no function
needs a frame pointer for it. (b) It exists, as C's VLA, with all the costs
above, and the stack analysis needs an annotated bound on every size.

The recommendation was (a), firmly: it simplifies most the interaction with
the dedicated SP, keeps worst-case stack use computable (a requirement for
the WCET and for putting the stack in a scratchpad), follows what critical
software and recent systems languages concluded, and, since nothing in
Haard needs it today, changes no existing program.

**Answer: (a)**, no dynamic stack allocation.

## 14. Guaranteed tail calls

**The question.** A **tail call** is a call that is the **last thing** a
function does: its result is returned directly, with nothing computed after.

```haard
def sum_to : u64
    @n : u64
    @acc : u64
    if n == 0:
        return acc
    return sum_to(n - 1, acc + n)      # a tail call: nothing happens after it
```

Each call normally pushes a **new frame**; `sum_to(1000000, 0)` would push a
million and overflow the stack. In a tail call the current frame is useless
after the call, so the compiler may **reuse** it: instead of "call, then
return" it **jumps** to the callee. The stack does not grow and the
recursion becomes, in effect, a loop.

**Optimisation or guarantee.** As an optimisation (a) the compiler **may**
do it when it can and when it pays (GCC and Clang often do at `-O2`), but a
program **may not rely on it**: another compiler version, or no
optimisation, and the stack may overflow. As a guarantee (b) the **language
promises** a tail call never grows the stack, so loops can be written as
recursion.

| Language | Rule |
|---|---|
| Scheme, ML, Haskell, Lua | **guaranteed** (Scheme's standard requires it) |
| Zig | guaranteed only when asked: `@call(.always_tail, f, args)` |
| Clang (C/C++) | guaranteed only with `[[clang::musttail]]` |
| Rust | not guaranteed; the word `become` is reserved for it, still in development |
| C, C++, Java, Go, Python | not guaranteed (Python refused explicitly: it loses the call history in error messages) |

**Why a guarantee is hard, in Haard especially.** Many calls that look like
tail calls are not:

1. **Destructors.** Locals with a destructor must be destroyed **after** the
   call returns:

   ```haard
   def f : i32
       let s = String("temp")
       return g(42)          # looks like a tail call, but s.destroy() runs after g
   ```

   With the cleanup stack this is common in Haard; record 0068's temporaries
   count too.
2. **Locals passed by reference.** If the function lends a `T&` to one of its
   locals, the frame must **stay alive** during the call and cannot be
   reused. In Haard `T&` is the normal way to pass without copying, so this
   is common too.
3. **Calling convention.** If the callee needs more stack space for
   arguments than the caller received, the frame does not fit; that is why
   Clang's `musttail` demands compatible signatures.
4. **The C++ back end.** `g++` does **not guarantee** tail calls. To honour
   (b) through C++, hdc would have to turn recursion into loops itself:
   simple for a function calling itself, but **mutual recursion** (`a` calls
   `b` calls `a`) needs techniques such as trampolines, which are slower.
   Portability through C++ would suffer.

**Real time.** Question 59 recommends forbidding recursion in real-time
code. A guaranteed tail recursion is at heart a loop, but one the WCET
analysis would have to recognise and bound like any other; writing the loop
(`while`, `for`) gives the same result with its bound in view.

**In the IR.** (a): nothing in the semantics; the machine-level optimiser
**may** turn a call into a jump when safe (no pending cleanups, no local lent
by reference, a compatible signature), and the stack analysis runs on the
final code, so it sees the effect. (b): a `tailcall` instruction with
verified rules (no pending cleanups, no local whose address is lent, a
compatible signature), the duty to honour it on **every** back end including
C++, and probably a syntax to ask for it, a new keyword or attribute.

The recommendation was (a): most Haard calls in tail position have
destructors or lent references, so the guarantee would hold in few cases;
the C++ back end cannot honour it in general; real-time code uses no
recursion anyway; it would likely need new syntax, which Hadley has
preferred to avoid; and it is additive, so an explicit form like Zig's can
come later.

**Answer: (a)**, only an optimisation.

## 15. How many registers, in how many banks

**The question.** Three things about the processor's register file: how
many **general-purpose** registers (integers and addresses); whether floating
point has a **bank of its own** or shares the integer registers; and, when
SIMD comes, whether vectors get a **third bank** or share floating point's.

**What it affects, and what not.** The IR does not change: it works on typed
values and unlimited virtual registers; the physical count matters only from
the machine IR down. What changes:

- **Register allocation.** Fewer registers, more values spilled to memory
  (a store, later a load). On the processor memory is a fixed-latency
  scratchpad, so the cost is predictable, but it exists and enters the WCET.
- **Calling convention** (question 31): how many arguments travel in
  registers and how many registers each function must save.
- **Interrupt and context-switch cost** (question 65): a handler may have to
  save every register it uses; more registers, a larger worst case.
- **Instruction encoding** (question 22): 32 registers cost 5 bits per
  operand, 15 bits for a three-operand instruction; 16 registers cost 4,
  leaving room for larger immediates or 16-bit compressed instructions.
- **On a PRET machine in particular**, a thread-interleaved pipeline (as
  PTARM and FlexPRET) needs **a full register file per hardware thread**, so
  the chip area spent on registers is multiplied by the thread count: the
  main argument against many registers or separate banks there.

| Processor | General purpose | Floating point | Vectors |
|---|---|---|---|
| x86-64 | 16 | 16 (XMM), **shared** with vectors | the same bank (16, or 32 with AVX-512) |
| ARM64 | 31 + a zero register | 32, **shared** with vectors | the same bank (V0–V31) |
| RISC-V | 32 (x0 always zero) | 32 **separate** (F/D) | 32 **separate** (V) |
| RISC-V "E" (embedded) | 16 | as above | as above |
| RISC-V Zfinx (embedded) | 32 | **none**: floats in the integer registers | — |
| MIPS, POWER | 32 | 32 separate | separate (POWER) |
| ARM Cortex-M (Thumb) | 16 (13 free) | 32 single precision, separate (with an FPU) | — |
| FlexPRET (PRET, RISC-V) | 32 **per thread** | — | — |

**The criteria.** *Count*: 32 is the common RISC point (RISC-V, ARM64,
MIPS, POWER), with small gains beyond it in typical code; 16 spills visibly
more (x86-64 suffers against ARM64) but saves encoding bits and area per
thread; more than 32 pays only with aggressive unrolling, software pipelining
or VLIW (Itanium had 128). *A separate floating-point bank*: the norm;
doubles the register count **without spending encoding bits**, since the bank
is implied by the opcode, and gives the FP unit its own read ports; against
it, moving a value between banks costs an instruction and the area doubles,
times the thread count; a unified file (RISC-V Zfinx, made for small
embedded cores) saves area and the compiler handles it well. *Vectors*:
sharing with floating point (x86, ARM) keeps a scalar float in lane 0 of a
vector register, so vectorising moves no data between banks; a separate bank
(RISC-V V) gives more registers and room for long vectors at the cost of
another file per thread.

**What the compiler needs.** Nothing here blocks the IR. For the processor's
back end: the count per bank, the reserved registers (question 20) and the
calling convention (question 31). The same allocator serves 16, 32 or a
unified file; only the target description changes.

The suggestion, being a hardware decision, was only a hint: 32 general
purpose per thread is the most tested point; floating point in a separate
bank if area allows, otherwise unified as Zfinx; vectors shared with
floating point tend to save area on a multi-threaded PRET.

**Answer: partly open.** The general-purpose count is still **16 or 32**.
With 32, the design follows RISC-V and ARM. With 16, there are 16
general-purpose registers **plus dedicated ones** (stack pointer, frame
pointer, thread pointer, instruction pointer, link register), probably close
to the address generation unit. Floating point gets a **separate bank**
(likely 16 alongside 16 GPRs). The vector bank is not decided.

A remark made with the answer: 16 general-purpose registers with the SP, FP,
TP, IP and link register **outside** them leaves 16 truly free registers --
a little more than x86-64, whose 16 include the stack pointer and usually the
frame pointer (14 or 15 free), but well below RISC-V (about 27 free after
zero, sp, gp, tp and ra) and ARM64 (about 29). Dedicated registers next to the address generation unit
also suit the compiler: locals addressed from SP or FP and thread-local data
from TP need no general register.

## 16. Can the dedicated SP be the base of a load or store?

**The question.** A load or store usually addresses **a base register plus
an offset**: `load r1, [base + 16]`. Can the dedicated stack pointer be that
base directly (`load r1, [sp + 16]`), or may only general-purpose registers,
so SP must be copied into one first?

**Why it matters so much.** Stack-relative accesses are among the most
frequent instructions of any program: locals in memory (in the decided
memory form every local with a slot lives there, and after mem2reg every
local whose address is taken stays there, common in Haard where `T&` is the
normal way to pass without copying, record 0068); spills (more frequent with
16 registers); prologues and epilogues saving and restoring registers;
arguments that did not fit in registers.

**The possibilities.**

- **(i) SP is a base directly**: one instruction. x86-64 does it
  (`mov eax, [rsp+16]`) and, more relevant here, **ARM64**, where SP is
  **not** a general-purpose register: code 31 in the base field means SP,
  and in other positions the zero register -- exactly a dedicated SP usable
  as a base. RISC-V allows it too, but there SP is an ordinary register (x2).
- **(ii) Only the dedicated FP may be a base**: each function does `fp = sp`
  at entry (one instruction, plus saving and restoring the old FP) and
  addresses locals as `[fp + offset]`: two or three instructions per call,
  workable, but small functions, which gain most from building no frame,
  always pay.
- **(iii) No dedicated register may be a base**: SP is copied into a
  general-purpose register first, either at every access (doubling the cost
  of every local) or once per function, keeping a general register busy
  with a copy of SP all along (15 left of 16). The worst for the compiler.

**The same holds for the other dedicated registers**, which Hadley placed
near the address generation unit:

| Register | What the compiler would use it as a base for |
|---|---|
| **SP** | locals, spills, arguments, saved registers |
| **FP** | locals, when a frame is built |
| **TP** (thread pointer) | each hardware thread's local data: `[tp + offset]` |
| **IP** (instruction pointer) | **globals and constants** relative to the code: `[ip + offset]`, PC-relative addressing, essential to position-independent code and to loading constants from a table near the code |

**A related detail: a local's address.** Passing a local by `T&` needs its
address **computed** into a general register, `r1 = sp + 16`, so an add that
**reads SP (and FP) as an operand** is needed too (ARM64's `add x0, sp,
#16`); without it every `T&` to a local costs a copy and an add. It overlaps
question 17, but this is where the need shows.

**What it affects.** Not the IR (it has slots; the back end chooses the
addressing). Instruction selection and frame layout directly; register
pressure (one fewer register under (iii)); the WCET (each extra instruction
has a fixed cost, multiplied by every stack access).

The suggestion (a hardware decision) was ARM64's model: SP, FP, TP and IP
usable as the base of loads and stores with an immediate offset, and an
instruction computing `register = SP/FP + immediate` to take a local's
address, so the dedicated registers are as useful for addressing as
general ones without occupying any of the 16.

**Answer: the ARM64 model**, as suggested.

## 17. Which instructions touch the SP

**The question.** With a dedicated SP, it changes only through the
instructions the ISA offers for it. Which will exist? The compiler needs some
operations on SP in every function and others in special situations.

**What the compiler does with SP.**

1. **Reserve and release each function's frame**: `sp = sp - frame_size` at
   entry, `sp = sp + frame_size` at exit. By answer 13 the size is always a
   compile-time **constant**, so adding or subtracting an immediate suffices.
   The important point is **how many bits** the immediate has:

   | Processor | SP adjustment in one instruction | Larger frames |
   |---|---|---|
   | ARM64 | 12 bits (up to 4 KB), optionally shifted by 12 (up to 16 MB in two) | load the constant into a register and subtract |
   | RISC-V | 12 bits signed (±2 KB) | `lui` + `add`, 2–3 instructions |
   | x86-64 | 32 bits | — |

   Embedded frames are usually a few hundred bytes and 12 bits cover almost
   all; larger frames need **`sp = sp - register`**.

2. **Save and restore registers** at entry and exit. `push` / `pop` (x86):
   compact, but each depends on the SP the previous one changed, and the
   offset of locals from SP moves through the function (tracked by the
   compiler, or avoided with FP). One adjustment, then stores at fixed
   offsets (RISC-V): simple and parallel, longer code. ARM64's middle way:
   `stp x29, x30, [sp, #-16]!` saves **two** registers and decrements SP in
   one instruction (store pair with pre-decrement). On the processor, a
   **thread-interleaved pipeline** makes the dependency between consecutive
   pushes free, and **code size** matters a lot since code lives in a
   scratchpad; prologues and epilogues repeat in every function, so compact
   forms save space.

3. **Build and tear down the frame with FP**: `fp = sp + immediate` at entry,
   `sp = fp + immediate` at exit, two operations between SP and FP directly.

4. **Copy between SP and general registers.** Reading SP: software stack
   overflow checks (compare with a limit), debugging. Writing SP from a
   register: at program start (pointing SP at the stack top, the start-up
   code), when creating each hardware thread's stack, and on context
   switches if any; it may be privileged, since ordinary code should not do
   it.

**A requirement of the stack analysis.** To compute worst-case stack use and
the WCET, the compiler must know **statically** each instruction's effect on
SP. Adding constants to SP (pre- and post-increment forms included) is
perfect; writing SP from a value computed at run time should happen only in
start-up code and thread creation, where the compiler knows what it is doing.

**What it affects.** Not the IR, which never mentions SP (answer 13). The
back end: prologues, epilogues, start-up code, thread creation, code size.

| Operation | Use |
|---|---|
| `sp = sp ± immediate` (how many bits?) | reserve and release the frame |
| `sp = sp - register` | frames larger than the immediate |
| `register = sp/fp + immediate` | a local's address (answer 16) |
| `fp = sp + immediate`, `sp = fp + immediate` | build and tear down the frame with FP |
| `register = sp`, `sp = register` | start-up, thread stacks, overflow checks |
| optional: `push` / `pop`, or load/store pair with pre/post-increment | compact prologues and epilogues |

The suggestion was ARM64 again, coherent with answer 16: an SP immediate of
at least 12 bits plus the register form for larger frames; direct
operations between SP, FP and general registers; and load/store pair with
pre-decrement and post-increment, or multi-register `push` / `pop` (as 32-bit
ARM's `push {r4-r7, lr}`), since code size weighs in scratchpads and the
dependency between them costs nothing in an interleaved pipeline.

**Answer: the ARM64 model**, with the note that the architecture will
probably support every operation presented here.

## 18. The frame pointer

**What it is.** The **frame pointer (FP)** points at a **fixed** place in
the current function's stack frame and does not move while it runs; SP may
move, FP stays. Usually each function's entry saves the old FP and the return
address side by side on the stack (a *frame record*) and points FP at that
pair:

```
             stack
   ┌───────────────────────────┐
   │ main's locals             │
   │ old FP     │ return addr  │ ◄── main's FP
   ├───────────────────────────┤
   │ control's locals          │
   │ main's FP  │ return addr  │ ◄── control's FP
   ├───────────────────────────┤
   │ read_sensor's locals      │
   │ control FP │ return addr  │ ◄── current FP
   └───────────────────────────┘
```

The FPs form a **linked list of frames**: following it, anyone can walk the
whole stack and know who called whom.

**What it was for, and why it stopped being required.** Historically, to
address locals: if SP changes during a function (`push` / `pop`, dynamic
stack allocation), locals' offsets from SP change, while from FP they stay
fixed. In Haard that is no longer needed: answer 13 rules out dynamic stack
allocation, SP can be adjusted once at entry and stay put, and answer 16 lets
SP be a load/store base, so locals can always be addressed from SP. That is
why compilers began **omitting** FP: GCC's `-fomit-frame-pointer` has been
the default with optimisation on x86 since about 2011, freeing RBP as one
more general register.

**Why it is coming back.** Without FP the **linked list of frames** is lost,
and walking the stack needs compiler-generated **tables** (DWARF unwind
tables) read by a complex routine. Hence a recent reversal: **Fedora 38
(2023)** and **Ubuntu 24.04** went back to compiling everything **with**
frame pointers so profilers can walk stacks cheaply, at a measured 1–2% on
x86, nearly all from losing a register; ARM64's ABI (AAPCS64) requires the
frame record on several platforms, and Apple makes it mandatory.

**What it would give Haard and the processor.** A **call history when the
program aborts**: by answers 3 and 9 and `unwrap`, several errors abort with
a message, and with FP the abort routine can print **who called whom** down
to the error by walking the list, with no table, very valuable for
diagnosing failures in the field. **No unwind tables**: by answer 4 there
are no exceptions, so no DWARF unwind tables are generated for them, and FP
gives the stack walk **for free** without spending scratchpad on tables.
**Debuggers and profilers** use the same list. Cost: on each call, save the
old FP + return address pair, point FP, restore at exit; with answer 17's
store pair, **two or three instructions per call**, a fixed, analysable WCET
cost.

**Options.** (a) A dedicated register, outside the general ones: keeping it
costs **no register**, only the prologue and epilogue instructions. (b) A
general register by convention (x29 on ARM64, s0 on RISC-V): costs a
general register when used. (c) None.

**What answer 15 already settled.** With 16 registers FP is **dedicated**
(a); with 32 the design follows RISC-V and ARM, a **general register by
convention** (b). What remained is a **compiler** decision: does it keep the
linked list of frames always, or only when needed? Always: call history on
abort, debugging and profiling always available, for 2–3 instructions per
call. Only when needed: saves those instructions and, with 32 registers,
frees FP as a general register, but loses the call history.

The recommendation was to keep the frame record always on both hardware
variants: with a dedicated FP it costs instructions and no register; with 32
registers losing one of 32 is barely felt (Fedora's case was one of 16 on
x86); the call history on abort is especially useful in a cyber-physical
system; it replaces unwind tables that would occupy scratchpad. It suggested
leaf functions could omit the record and a compile option could turn it off.

**Answer: keep the frame record always.** The leaf-function exception and
the opt-out option were not taken.

## 19. Where the return address goes

**The question.** When a function is called the processor must remember
**where to return**. Two ways:

- **(a) In a register, the link register (LR)**: ARM, RISC-V, MIPS, POWER.

  ```
  bl  read_sensor    ; jumps and puts the return address in LR
  ret                ; jumps to the address in LR
  ```

- **(b) On the stack, pushed by the call instruction itself**: x86.

  ```
  call read_sensor   ; pushes the return address (a memory write) and jumps
  ret                ; pops it (a memory read) and jumps
  ```

**Differences that matter.**

1. **Leaf functions are cheaper with LR.** A function that calls nobody gets
   the address in LR and returns through it **without touching memory**; in
   (b) **every** call writes and reads memory, even a three-instruction one.
2. **Functions that call others must save LR**, or the next call overwrites
   it. Answer 18 fits here: the frame record is exactly the **FP + LR** pair,
   and with answer 17's store pair both are saved in one instruction (ARM64's
   `stp x29, x30, [sp, #-16]!`); with LR the frame record comes almost free.
3. **Interrupts.** With LR, an interrupt arriving while a function uses LR
   cannot keep its own return address in the **same** LR without destroying
   the interrupted function's; ARM solves it with a **separate** register for
   the exception return address (ELR), which ties to question 65. In (b) the
   interrupt simply pushes.
4. **Security.** A return address on the stack is the classic buffer-overflow
   target (overwriting it hijacks the program, the basis of ROP attacks).
   With LR, leaf functions are immune; others still save LR on the stack, but
   answer 9's bounds checks close most of that door.
5. **WCET and stack.** In both models each call's effect on the stack is
   statically known; (b) adds a memory access to every call and return, a
   fixed but ever-present cost.

**Instructions the compiler needs with it.** A **direct call** (`bl
target`) with some number of offset bits, which matters because of
**scratchpads and overlays**: functions in different memories may sit far
apart, beyond a direct call's reach, and then the compiler inserts a small
intermediate stub (a *veneer* or trampoline) or calls through a register (the
range is question 22). A **call through a register** (`blr r3`),
indispensable in Haard for **virtual methods** (`vcall`), **closures**
(record 0058) and **methods as values** (record 0071). A **return** (`ret`,
jumping to LR). **Saving and restoring LR** with FP (answer 17's store pair),
and copying LR to and from a general register (rare cases).

**What answer 15 already said.** The **link register** is among the
dedicated registers of the 16-register variant, which is option (a) with a
dedicated LR; with 32, following RISC-V and ARM, LR is a general register by
convention (x30 on ARM64, x1 / `ra` on RISC-V).

**What it affects.** Not the IR, which has `call`, `vcall` and `ret`; how
the return address is kept is the back end's business: prologue and epilogue
(the FP + LR frame record), indirect calls, veneers for distant calls,
interrupt handling.

The suggestion was (a), coherent with the ARM64 model chosen in answers 16
and 17, plus a call through a register, a separate register for an
interrupt's return address, and a direct-call range covering a scratchpad's
code space with veneers for the rest.

**Answer: the ARM64 model**, a link register, with the instructions above.

## 20. Other dedicated registers

**Already settled.** Answer 15 listed SP, FP, TP, IP and the link register
as dedicated (in the 16-register variant), and answer 16 made **IP readable
as an address base**; so "a readable PC" and "a thread pointer" were
answered. Three remained: a **zero register**, **flags**, a **global
pointer**.

**1. A zero register** always reads 0, and writing it has no effect (RISC-V
x0, ARM64 xzr, MIPS $0). Uses: `r1 = 0` as `r1 = zr + zr` with no special
instruction; **storing zero** without first putting 0 in a register (common
in Haard when initialising fields); comparing with zero; discarding a result
(a subtraction into zero is a compare). Cost: one slot of the register
encoding, irrelevant with 32 registers, painful with 16. ARM64's trick (code
31 means SP as an address base and zero elsewhere) does not help the
16-register variant, whose SP is dedicated outside the 4-bit field. Without
one (as x86), everything above is done with immediates at a few extra
instructions: a convenience for the compiler, not a need.

**2. Flags, or comparisons into a register.** How a comparison's result
reaches a branch.

- **Flags** (x86, ARM): `cmp r1, r2` sets N, Z, C, V in a flags register and
  `b.lt` reads them. For: add and subtract **with carry** (`adc`) in one
  instruction, making `mul_wide` and any multi-word arithmetic cheap
  (question 12); cheap overflow detection. Against: flags are **hidden
  state**, every instruction that sets them destroys the previous value and
  the compiler must mind the order (ARM chooses which instructions set them
  with the `S` suffix); one more register per hardware thread, which
  interrupts must save.
- **Comparison into a register** (RISC-V, MIPS): `slt r3, r1, r2` gives 0 or
  1, or `blt r1, r2, label` compares and branches in one instruction. For: no
  hidden state; a comparison's result is a value like any other, exactly as
  in the IR (`%c = lt %i, %n`, then `br %c`); compare-and-branch is one
  instruction. Against: add with carry costs 3–4 instructions; a
  compare-and-branch spends bits on two registers, leaving less branch range.
- **ARM64 does both**: flags (`cmp`, `b.lt`, `adc`) plus branches on a
  register being zero or not (`cbz` / `cbnz`), test-bit-and-branch (`tbz`),
  and conditional select (`csel`, question 24).

Neither the WCET (every option has a fixed cost) nor the IR (comparisons
produce `bool` values; instruction selection maps them) depends on it.

**3. A global pointer (GP)** points into the middle of the global data area
so any nearby global is read in one instruction, `load r1, [gp + offset]`
(RISC-V gp, MIPS $gp, Itanium). The alternative is IP-relative addressing,
already available by answer 16 (`load r1, [ip + offset]`, as x86-64 and
ARM64 do), which usually makes GP unnecessary. **But scratchpads change
that**: code lives in the instruction scratchpad and data in the data
scratchpad, possibly at **very distant addresses** (instructions at
`0x0000_0000`, data at `0x8000_0000`); IP-relative addressing works in one
instruction only if the distance fits the offset, otherwise each global
access costs two or three. A GP pointing into the data scratchpad keeps the
globals a short offset away **wherever the code is**. Alternatives: an
IP-relative mode with a **large range in two instructions** (ARM64's `adrp`,
±4 GB), or loading the data area's address into a general register when
needed.

The suggestion: a zero register with 32 registers, none with 16; ARM64's
model for comparisons (flags plus `cbz` / `cbnz` and a conditional select);
a dedicated GP because of the split between code and data scratchpads, or a
two-instruction wide IP-relative mode.

**Answer.** **No zero register.** A **dedicated branch comparing with
zero**. Branches in the **RISC-V / MIPS style** (compare and branch in one
instruction, no flags for branching). The architecture **has add with
carry**. **GP is a dedicated register**, like SP and FP. Open detail: where
an add-with-carry keeps its carry (a single carry bit, or a register).

## 21. Stack direction, alignment and overflow

Three questions about the stack. None touches the IR, which never mentions
SP (answer 13); all touch the back end and the memory layout.

**1. Which way it grows.** **Down** (decreasing addresses) is nearly
universal: x86, ARM, RISC-V, MIPS; a push decrements SP then writes, and
locals sit at **positive** offsets from SP (`[sp + 16]`). **Up** is rare: HP
PA-RISC, the 8051, some DSPs. What changes: the classic single-memory layout
(stack from the top going down, heap and data from the bottom going up,
meeting in the middle), which matters less with the stack in its own
scratchpad (question 41); **where an overflow lands** (below the stack's
region when growing down, above when growing up), which decides which data
is corrupted if nothing detects it; and a **local array overrun**, which
moves toward **higher** addresses, hitting the **caller's** frame and saved
return address when the stack grows down, unused space when it grows up -- a
security argument for up, though answer 9's bounds checks already cover
Haard's arrays. For the compiler either works; without a strong reason,
down keeps everything as tools and literature assume.

**2. SP alignment.**

| Processor | Rule |
|---|---|
| ARM64 | **16 bytes**, checked by hardware: a misaligned SP used as a base faults |
| x86-64 | 16 bytes at a call, by ABI convention |
| RISC-V | 16 bytes, by convention |

Why 16: 8-byte values (`i64`, `f64`, pointers) want 8-byte alignment
(question 26), 128-bit SIMD values and answer 17's store pair of two 64-bit
registers want 16; with SP 16-aligned at every function entry, the compiler
places everything at fixed offsets with no run-time adjustment. Cost: frames
round up, a few extra bytes of (scratchpad) stack per call. Care for future
SIMD: with 256-bit or wider registers (question 46), vectors on the stack
want 32 or more, so either SP starts aligned to that or the compiler
realigns the frames that need it (which needs FP, kept always by answer 18);
changing the ABI's alignment later breaks compatibility with compiled code,
so it is worth choosing with SIMD in mind.

**3. Hardware overflow detection.** Without it, a stack past its region's
limit silently writes over its neighbour; without an MMU not even Linux's
guard page exists. Ways to detect: a **stack-limit register** (ARMv8-M's
`MSPLIM` / `PSPLIM`, modern Cortex-M microcontrollers) compared with SP on
every change, faulting when crossed, **zero instructions**, constant time,
one per hardware thread; **guard regions in an MPU** (a forbidden range just
past the stack); **software checks**, the compiler comparing SP with the
limit in every prologue (GCC's `-fstack-limit-register`), 2–3 instructions
per call. What Haard already guarantees: fixed frames (answer 13), and if
question 59 forbids recursion in real-time code, the compiler **proves** at
compile time that real-time code never overflows (the worst case is
computed). Detection is then a **safety net** for code that is not real-time
(where recursion may be allowed) and for an error in the analysis or the
stack configuration. When detected, an overflow should **abort with a
message**, coherent with answers 3 and 9, through the abort routine
(question 29).

The suggestion: down; 16 bytes checked by hardware as on ARM64, or the
largest planned SIMD register if wider; a **limit register per thread**, as
ARMv8-M's, cheap in hardware, free in instructions, harmless to the WCET;
without it, prologue checks only in functions whose stack use cannot be
proven.

**Answer: as suggested**: grows down, 16-byte alignment, a stack-limit
register.

## 22. Instruction encoding

**The question.** Two things: the **length of instructions** (all the same,
varied, or fixed with a compressed form), and **how many bits the
immediates have** (constants inside the instruction) in loads and stores,
arithmetic and branches. The IR does not change; the back end changes a lot,
and there is an important indirect effect here: **code size**, since code
lives in a scratchpad.

**1. Instruction length.**

- **Fixed** (ARM64, base RISC-V, MIPS: all 32 bits): simple decoding;
  **constant fetch time per instruction**, good for a PRET pipeline and the
  WCET; code size by counting instructions; simple branch addresses. Against:
  limited immediates, so large constants and distant branches become
  sequences; larger code.
- **Variable** (x86, 1 to 15 bytes): dense code, any 32- or 64-bit constant
  inline. Against: complex decoding; a branch's size depends on the distance,
  which depends on the sizes in between, so the assembler iterates until it
  settles (*branch relaxation*); **fetch time varies** with whether an
  instruction crosses a fetched word, which hurts the WCET. Bad for a PRET.
- **Fixed with a compressed form** (RISC-V with the C extension, 16- and
  32-bit; ARM Thumb-2): the commonest instructions, with the most used
  registers and small immediates, get a 16-bit version, and code shrinks by
  **25–30%**; the compiler picks the short form when operands fit. Against: a
  32-bit instruction may start mid-word and cross the fetch boundary, so
  fetch time is no longer always the same unless the hardware fetches enough
  per cycle never to stall; a moderate complication for the WCET.

Why density weighs here: with code in a scratchpad, smaller code means
**more functions fit**, fewer overlays (question 39) and fewer run-time code
copies, which cost WCET time.

**2. Immediates, and what the compiler does with each.**

| Use | For | References | Out of range |
|---|---|---|---|
| **Load/store offset** | locals (`[sp+16]`), fields (`[r1+8]`), globals (`[gp+...]`) | RISC-V: 12 bits signed (±2 KB). ARM64: 12 bits unsigned **scaled by the access size** (×8 for a 64-bit load: up to 32 KB) | add the offset into a register first |
| **Arithmetic** | `i + 1`, `x & 0xFF`, comparisons with constants | 12 bits is common and covers most real constants; ARM64 also encodes **bit masks** (`0x00FF00FF...`) in a special format | build the constant in a register |
| **Large constants** | addresses, arbitrary `u64` | ARM64: 16-bit chunks (`movz` + up to 3 `movk`). RISC-V: up to 8 instructions, or a load from an IP-relative **constant table** | — (question 30) |
| **Conditional branch** | `if`, loops | RISC-V: 12 bits (±4 KB); the RISC-V style chosen (compare two registers and branch) leaves fewer bits for the target | invert the condition and jump over an unconditional branch |
| **Unconditional jump and call** | jumps, function calls | RISC-V `jal`: 20 bits (±1 MB). ARM64 `b` / `bl`: 26 bits (±128 MB) | a veneer or a call through a register (answer 19) |

A detail that multiplies range: if instructions are always 4-byte aligned,
branch offsets count **instructions**, not bytes, so 12 offset bits reach
±8 KB rather than ±2 KB; with a compressed form the alignment is 2 and the
gain smaller. The scratchpad connection: if the direct-call range covers the
whole instruction scratchpad, no internal call needs a veneer; calls into
another memory or an overlay may.

**What it affects in the compiler.** Legalisation (splitting constants and
offsets that do not fit); fixing out-of-range branches (simple with fixed
length, iterative with a compressed form or variable length); code size,
which feeds scratchpad placement, overlays and the WCET.

The suggestion, following ARM64: fixed 32-bit instructions (ARM64 has no
compressed form), keeping fetch time constant, with a 16-bit form to study
later if scratchpad space gets tight; 12-bit load/store offsets **scaled** by
the access size; 12-bit arithmetic immediates; conditional branches with
whatever remains after two registers, counted in instructions; calls and
jumps covering at least the whole instruction scratchpad (20–26 bits); large
constants in 16-bit chunks or loaded IP-relative (question 30).

**Answer: instructions of 32 or 16 bits** (a compressed form beside the
32-bit one), rather than the suggested fixed 32 bits. Immediates:
**12 or 16 bits, still open; at least 12**.

Left open after the answer, asked back: the ranges of conditional branches
and calls/jumps; whether load/store offsets are scaled by the access size;
whether a 32-bit instruction may start at a 2-byte boundary (crossing a fetch
word, which matters to the WCET); and which registers and immediates the
16-bit forms reach.

## 23. The kind of pipeline

**The question.** How the processor organises execution over time, which
decides **how much the compiler must know about the hardware** to produce
code that is correct, fast and WCET-analysable. The IR does not change; the
back end and the WCET's timing model do.

**(a) Thread-interleaved** (the classic PRET model). **N hardware threads**,
and each cycle a **different** thread issues an instruction, round robin.
With N at least the pipeline depth, **two instructions of one thread are
never in the pipeline together**: no data hazards between neighbours (no
forwarding, no stalls), no branch prediction (a branch is resolved before
the thread's next fetch), and **every instruction takes the same time** (one
every N cycles per thread). Examples: **PTARM** (4 threads, 5 stages) and
**FlexPRET** (RISC-V), which is more flexible: a hard real-time thread may
get more slots, and a thread issuing in consecutive cycles brings hazards
back for the hardware to handle. For the compiler the simplest case:
instruction order does not change timing, so **no scheduling is needed**, and
the WCET is basically counting instructions. The price: **one thread alone
runs at 1/N of the clock**; performance comes from using the N threads,
which needs the language or environment to spread tasks over them
(question 63).

**(b) Scalar in-order** (the classic 5-stage pipeline). One instruction per
cycle from one thread; neighbours overlap, so hazards appear (a use right
after a load **waits a cycle**, a taken branch costs fixed cycles without
prediction). The compiler gains by **scheduling** (something useful between
a load and its use); the **WCET needs a pipeline model**, since an
instruction's time depends on its neighbours -- still predictable (no
prediction, no out-of-order), only more work to analyse.

**(c) VLIW.** The compiler groups independent instructions into a **bundle**
that executes at once on parallel units; the hardware decides nothing, **all
parallelism is scheduled by the compiler**. Real-time example: **Patmos**
(T-CREST), a two-way VLIW chosen precisely for performance **with** full
predictability. For the compiler the **most work**: a scheduler, bundling,
predication to remove branches, *software pipelining* (the same technique as
mapping loops onto the CGRA), more registers to keep operations in flight.
In exchange, a very precise WCET.

**(d) Other**, such as superscalar out-of-order, against the PRET idea:
timing depends on hidden hardware state.

**Exposed pipeline or interlocks.** With **interlocks** the hardware
detects hazards and **waits by itself**; scheduling is optional, code is
correct anyway, and binaries survive future processors with other pipelines.
With an **exposed** pipeline the hardware **does not check**, and the
compiler **must** ensure a result is used only when ready, inserting `nop`s
or reordering, possibly with **delay slots** (the instruction after a branch
**always runs**, as on the original MIPS and SPARC); simpler hardware, but
code grows with `nop`s, the compiler must know latencies **exactly**, and
**changing the pipeline breaks old binaries**. In (a), with enough threads,
the question nearly vanishes (no hazards within a thread); it returns if a
thread can issue in consecutive cycles, as in FlexPRET.

| | (a) interleaved | (b) in-order | (c) VLIW |
|---|---|---|---|
| Instruction scheduling | unnecessary | useful | mandatory |
| WCET timing model | a cost per instruction | table + hazard model | a cost per bundle |
| Back-end complexity | low | medium | high |
| One thread's performance | 1/N | 1 instruction/cycle | several/cycle |

The suggestion was (a), with interlocks for a thread issuing in consecutive
cycles and no delay slots.

**Answer: thread-interleaved, and a thread may fetch more than one
instruction in parallel.** The **compiler** guarantees that instructions
issued together can run in parallel; each instruction carries a **dedicated
bit**, set by the compiler, saying whether it may execute in parallel with
its neighbour.

**What follows from the answer.** This combines (a) with explicit
instruction-level parallelism in the style of (c), as TI's C6000 VLIW (its
"p-bit" marks an instruction that runs in parallel with the next) and
Qualcomm's Hexagon (end-of-packet bits) do. So:

- the back end needs a **scheduler and bundler**: dependence analysis
  (an instruction may not read what another in its bundle writes, unless the
  bundle's semantics say reads happen first), resource checks (how many of
  each functional unit), and later software pipelining and predication
  (question 24) to fill bundles;
- the hardware does **not** check bundles, so a wrong bit is a silent wrong
  result: the back end needs a **bundle verifier**, and a machine-level
  simulator of the processor that honours bundle semantics is the oracle for
  it (the IR interpreter runs the IR, not machine code);
- the WCET stays simple: a bundle has a fixed cost;
- the bit costs one bit of every instruction's encoding, the 16-bit forms
  included (question 22);
- **a correct first back end can clear the bit everywhere**, issuing one
  instruction at a time; bundling is then an optimisation, added and tested
  on its own.

Open details: the maximum instructions per bundle; which combinations are
allowed (how many memory, branch, floating-point operations per bundle);
the semantics inside a bundle (all reads before any write?); whether one
thread can issue bundles in consecutive cycles (interlocks between
bundles?); whether 16- and 32-bit instructions mix in one bundle.

## 24. Conditional execution

**The question.** Besides a conditional branch, can the processor do
something conditionally **without branching**? For `if a < b: x = a else
x = b`, a branch makes the time depend on the path taken; a select
(`x = c ? a : b`) takes the same time either way. On this processor that
matters for three reasons: **single-path / constant-time code** (question
66), the central technique of **Patmos**; **filling bundles** (answer 23),
since the scheduler works inside a basic block and every branch cuts one
(if-conversion lengthens blocks, which is why C6000, Hexagon and Itanium
invest in it); and **not** raw speed, since in a thread-interleaved
pipeline a branch is nearly free (it resolves before the thread's next
fetch).

**(a) Branches only.** Simplest hardware. A select can be built from
arithmetic when a compare writes 0/1 into a register (`slt`):
`m = -c; x = (a & m) | (b & ~m)`, about four instructions. It cannot make a
**side effect** (a store) conditional.

**(b) A select / conditional move** (x86 `cmov`, ARM64 `csel`, MIPS
`movz`/`movn`, RISC-V Zicond), e.g. `sel rd, rc, ra, rb`. The cost is a
**third source operand**, so a third register-file read port, growing with
bundles. ARM64 avoids it by taking the condition from flags, which this
processor does not have (answer 20). Zicond avoids it with `czero.eqz`,
`czero.nez` and an `or`: two reads each, but three instructions per select
(two cycles if the `czero`s share a bundle). A select chooses **values**;
a conditional store still needs a branch, or the trick of always storing
(to the right address or to a dummy one, or writing the old value back).

**(c) Full predication**: predicate registers and (nearly) every
instruction conditional -- ARM32 (4-bit condition field, dropped in ARM64
for its encoding cost), Itanium (64 predicates), Hexagon (4), C6000,
**Patmos** (8 predicates, every instruction predicated, built for
single-path code and WCET). It covers everything, conditional loads and
stores included, and if-conversion becomes trivial (both sides of an `if`
in one bundle under opposite predicates). The cost: 3-4 bits in every
instruction, very tight in the **16-bit** forms (answer 22); a predicate
bank and compares that write it; predicate allocation and real
if-conversion in the back end.

**(d) A middle way.** (d1) a select plus a **conditional store** only;
(d2) a Thumb-2 style `IT` prefix making the next few instructions
conditional with no bits in them (awkward with bundles); (d3) predication
only on the 32-bit forms.

**What the IR sees: little.** The IR has a `select` in every case (the
if-conversion target; x86 and ARM map it directly; Haard has no
if-expression, so it comes from optimisation or the single-path
transform). Predication does **not** rise into the IR: as in LLVM for
Hexagon, it appears after instruction selection. The question decides the
ISA, the back end and what the timing analysis can do.

The suggestion was (d1), a 3-source select plus a conditional store,
deciding full predication together with question 66: (c), probably on the
32-bit forms only, if single-path code is central; (d1) if it is
occasional.

**Answer: open.** Not decidable yet; it waits on 66. The IR is not
affected.

## 78. One IR, two control forms

**The question.** Control flow can be held in two ways: **structured**,
nested regions (`if`, `loop`, `for`, `block`) with `break` and `continue`,
as the source is written (WebAssembly); or a **CFG**, basic blocks joined by
jumps (`br`, `cond_br`, `switch`), with block parameters instead of φ (LLVM,
QBE, Cranelift). The proposal: **one** instruction set, type table, printer
and verifier, with control in one of the two forms. The lowering produces
the structured form; a **flatten** pass turns it into the CFG; below that,
each back end has its own machine IR.

**Why two.** Each form suits a kind of work:

| Work | Needs to know | Natural form |
|---|---|---|
| A loop bound for the WCET (84) | that this is a loop, and how many times it runs | structured |
| Vectorisation, CGRA kernels, DMA double buffering | induction variable, step, trip count | structured |
| Splitting functions for overlays | single-entry, single-exit regions | structured |
| mem2reg, constant propagation, dead code | dominators, merges | CFG |
| Instruction selection, register allocation | blocks and edges | CFG |
| The interpreter (87) | a plain `pc = next block` loop | CFG |

In a CFG a loop is only a cycle of blocks; vectorising means **recovering**
the structure (LLVM's LoopInfo and ScalarEvolution, among its heaviest
parts). For the WCET, each loop's bound must survive every pass: in the
structured form it is written on the region itself; in a CFG it is metadata
on a header block, which any pass may drop.

**(a) CFG only, with loop annotations** (LLVM). One form, every pass written
once; but a loop analysis to write, metadata every pass must preserve (LLVM
loses loop metadata often), and vectorising or extracting kernels the hard
way.

**(b) Structured only** (WebAssembly, structured SPIR-V). No flatten; but
mem2reg, scalar optimisations and instruction selection are awkward (SSA
over regions needs region arguments and results everywhere, as MLIR `scf`
does), and so is the interpreter.

**(c) Two separate IRs** (HIR/LIR; MLIR's `scf` → `cf`; Swift SIL → LLVM).
Each clean, but two builders, printers and verifiers and a translation
between them. This was §3.3 of designing-the-ir.md, dropped because with
the low IR typed and reading fields by name, the **only** difference left
would be the shape of control.

**(d) One IR, two control forms** (the proposal). Instructions, types and
printer shared; only control changes. The cost: a verifier that knows both,
a flatten pass (easy: each region becomes 2-3 blocks), and loop passes that
must run **before** flatten.

The proposal argued that Haard has no `goto`, so every Haard program is
already structured and the lowering is direct (`while` → `loop`, a range
`for` → `for`, `switch` → nested `block`s), never having to rebuild
structure out of jumps (Emscripten's "relooper", the hard part of the
structured model in languages with `goto`).

Three details fixed by approving it:
1. **A function is in one form at a time**; no regions inside blocks, as
   MLIR allows. Simpler verifier and passes.
2. **Flatten is one way**; there is no pass back from CFG to structured.
   Passes needing loops (vectorisation, CGRA, overlay splitting, collecting
   flow facts) run before it; a CFG pass that needs loops later (hoisting
   invariant code) uses natural-loop analysis, cheap over dominators. Flatten
   copies each loop's **bound** onto its header as a flow fact, and the
   verifier keeps checking it is there (84).
3. **The interpreter runs only the CFG form** (87); the structured form is
   always tested through flatten, and a flatten bug shows as a difference
   from the C++ path, so the oracle covers both.

Recommendation: (d), with the three details.

**Correction: Haard had `goto`.** Hadley asked to check, and it did:
`label name` and `goto name` (record 0063), checked by the StatementChecker
(a label of the same body, written once; backward always fine; forward may
not land where a skipped declaration is in view) and emitted as C++'s
`goto`. Nothing forbade jumping **into** a block, a loop included, or two
`goto` loops crossing, so a Haard program could have **irreducible**
control flow, a loop with two entries:

```
if x:
    goto middle
label top
a()
label middle
b()
if c:
    goto top
```

No nesting of regions holds that. `break` and `continue` took no label, so
leaving nested loops was only possible with `goto`. (`yield` is also
scanned and parsed, but nothing past the parser handles it; if it ever
becomes generators, the IR will need coroutines.)

The options then:

- **(a) A function using `goto` is lowered straight to the CFG form.**
  Functions without it, nearly all, keep the structured path; those with it
  stay out of loop passes, their loops found by CFG analysis, and an
  irreducible one needs a written bound or is refused in real-time code.
  Cost: the lowering emits control two ways (expressions, calls and the
  cleanup stack shared; only the ~6 control statements doubled).
- **(b) The structured form gets named `block`/`loop` and `br name`**
  (WebAssembly style); the common uses of `goto` (leaving nested loops,
  "try again") fit, and only irreducible ones fall back to (a).
- **(c) Change the language**: remove `goto`, possibly adding a labelled
  `break`/`continue`. Haard is then fully structured and question 78 is
  exactly as first stated.

The recommendation was (a), keeping `goto` and reaching (b) later as an
optimisation. Either way, a `goto` leaving a scope must run the
destructors of what is in it, as C++ does.

**Answer: (d) approved, and `goto` and `label` removed** (option (c), record
[0078](../0078-goto-and-label-are-removed.md)), so every function is
structured. A labelled `break` was not added; whether to is open.

## 79. Signedness: in the type or in the instruction?

**The question.** A 32-bit register holds the same bits for an `i32` and a
`u32`; only some operations differ. (a) **In the type** (the original
proposal, designing-the-ir.md §3.4): values are `i32` or `u32` and `div`
picks from its operands. (b) **In the instruction**: types carry only the
width, and the sign-dependent operations come in two versions.

**What depends on the sign.** With overflow wrapping (1a), `add`, `sub`,
`mul`, `and`, `or`, `xor`, `shl`, `==` and `!=` give the same bits either
way. Only these differ:

| Operation | Signed | Unsigned |
|---|---|---|
| divide, remainder | `sdiv`, `srem` (with `MIN / -1`, 3a) | `udiv`, `urem` |
| shift right | sign fill (2b) | zero fill |
| `<` `<=` `>` `>=` | `slt`… | `ult`… |
| widening | `sext` | `zext` |
| integer ↔ float | `sitofp`, `fptosi` | `uitofp`, `fptoui` |
| high half of `mul_wide` (12a) | signed | unsigned |
| the shift count (2b) | negative counts as too large | -- |

About 8 pairs out of ~40 operations.

**How others do it.** LLVM, Cranelift, QBE, WebAssembly and GCC's RTL keep
the sign in the instruction; LLVM had signed types (`sbyte`, `ubyte`, up to
2.0, 2007) and **removed** them. MLIR, after a long debate, made `arith`
signless, keeping signed types only in high-level dialects near the front
end. SPIR-V is a hybrid: the type declares a sign but the instructions are
explicit (`OpSDiv`/`OpUDiv`) and the type's sign is mostly ignored.

**What (a) costs.** No-op conversions: `x as u32` on an `i32` becomes a
`cast` that changes no bit but sits in the IR, and passes trip on it (common
subexpressions do not see that `x` and `x as u32` are the same bits;
instruction selection patterns must look through it). Every rule is written
per type: the verifier checks `add` takes two `i32` or two `u32`, never one
of each; every pattern-matching optimisation considers both variants. And
the machine has no sign: registers, instruction selection and the
processor's ISA speak in widths, so the sign has to be erased anyway, later.

**What (a) gives.** A readable `--emit-ir` showing source types, and a
verifier that catches a lowering that mixes `i32` and `u32`. The second is
small in practice: in (b) the choice between `sdiv` and `udiv` is made in
**one place** of the lowering, from the Haard type the ResolutionTable
already gives, and a wrong choice is caught by the oracle (the IR
interpreter disagrees with the C++ path). The source type for a debugger
comes from the mapping of 86, not from the IR type.

Nothing else depends on it: vectorisation, the CGRA, WCET and placement are
indifferent. It is about the simplicity of the IR and its passes.

**Details with (b).**
1. **Type names**: not `i32`, which in Haard **is** signed. Proposed `b8`,
   `b16`, `b32`, `b64` ("bits"), with `bool`, `f32`, `f64`, `ptr`.
2. **`char`** is `b8`; **`isize`/`usize`** take the target's width in the
   lowering (`b64` on every target today, read from the target table).
3. **A conversion between `i32` and `u32` disappears**: the same value is
   used. Widening and narrowing are `sext`/`zext`/`trunc`, as in LLVM.

**Recommendation: (b)**, overturning my own proposal: (a) came from
"Haard's model", but the IR talks to the machine, and LLVM's and MLIR's
experience was precisely to take the sign out of the type. A conscious
exception to keeping the IR as high level as possible: its level comes from
struct and class types, fields by name, `vcall` and regions, not from the
sign of an integer.

**Answer: (b)**, with the three details.

## 80. Pointers, fields and the layout table

**The question.** Three parts: **typed pointers** (`ptr<Point>`); **fields
by name and elements by index** (`field %p, Point.x`, `index %p, %i`), no
offsets in instructions; **explicit tables** of structs, classes and
vtables, with the **layout** (size, alignment, field offsets, endianness)
computed per target and read only by instruction selection and the
interpreter. The second and third are the heart of a high-level IR and
stay. The first changes, for the reason of 79.

**(a) Typed `ptr<T>`** (the original proposal). **(b) Opaque `ptr`**, the
type written on the instruction: `load b32, %p`, `field %p, Point.x`,
`index %p, %i : f32`. This is what **LLVM did in versions 15-17
(2022-23)**: twenty years of typed pointers, migrated to opaque `ptr`.

LLVM's reasons hold for Haard:
- **No-op bitcasts.** Haard converts any pointer to any other (`p as i8*`,
  record 0049), down a hierarchy unchecked, and through a `union`. With
  typed pointers each is a no-op instruction in the way of passes, like
  `i32`/`u32` in 79.
- **The pointer's type says nothing extra**: the operation (`load`,
  `field`, `index`) names the type it needs anyway.
- **It does not even help alias analysis in Haard**: 6a ruled out
  type-based aliasing.

Lost: the verifier no longer checks that a `field %p, Point.x` got a
pointer that came from a `Point`. LLVM accepted that; the interpreter oracle
covers it.

**The reservation for 35 fits here**: the pointer carries an **address
space**, `ptr` (generic, the default) or e.g. `ptr(spm0)`. Always generic
for now.

**Details.**
1. **Aggregates are never values.** Struct, class, tuple and fixed array
   always live in memory, in a slot; IR values are scalars only (`bool`,
   `bN`, `fN`, `ptr`, `vec` later). A by-value argument is a copy made by
   the caller (with the copy `init`, record 0038) and its pointer is
   passed; a by-value result gets a **destination pointer** (record 0045's
   temporary written in place). Splitting small structs into registers is
   the ABI's job in instruction selection, and later SROA's (mem2reg for
   aggregates). LLVM allows aggregate values and advises against them;
   forbidding them simplifies. So `mul_wide` (12a), which gives back a
   tuple, becomes `mul` + `smulhi`/`umulhi`, and no instruction has two
   results.
2. **Address instructions**: `field`, `index`, `upcast D→B`, `downcast
   B→D`, each offset resolved only by the layout. `upcast` exists for a
   concrete reason: a class may derive from a struct (record 0065); the
   class has a vptr and the struct has not, so the base is **not at offset
   0** and going up adds an offset. C++ does it silently today; the IR has
   to write it.
3. **Tables**: struct, class (base, fields, vtable slots, destructor),
   union, fixed array. **Enum and tuple are not kinds of their own**: the
   lowering turns them into what the emitter already makes, a `{tag;
   union}` struct and a struct per shape (decided 2026-10-07). A `switch` on
   an enum reaches the IR as a `switch` on the tag.
4. **Layout rules**: fields **in declaration order, never reordered**
   (predictable for memory-mapped registers and DMA); natural alignment
   with padding; **vptr first** in a class, as in C++; all computed by
   **one function** from the tables and the target. Haard's `sizeof` is a
   `size T` constant resolved by the target's layout.

**Recommendation: (b), opaque pointer with an address space**, plus
details 1-4.

**Answer: (b)**, with details 1-4.

## 81. Everything in memory is a named entity

**The question.** Every thing that occupies memory is a **named object** in
the IR: each function's code, each global, each literal (strings, constant
`{1,2,3}`, record 0041's symbol table), each vtable, each stack.
Instructions refer to them **by symbol** (`call @f`, `addr @str.0`), never
by a numeric address. **Where** each object lives (which scratchpad, which
part of main memory, which overlay group) is in a **separate table**,
applied only at final layout, so **analyse → place → analyse again** runs
without redoing instruction selection.

**Why it matters on this processor.** With no cache and scratchpads, where
an object lives decides whether the program fits and how long it takes (a
scratchpad read is 1 cycle, main memory N). The classic approach (WCC at
Dortmund, Suhendra et al.) iterates: place, compute the WCET, move what is
on the worst path into the scratchpad, repeat. Placement must be **data
that is recomputed**, not something written into each object.

**(a) An attribute on the object** (LLVM `section`, GCC
`__attribute__((section(".spm")))`), fixed when the object is made, by the
programmer or a heuristic; changing it means rewriting attributes, and
iterating with the WCET is awkward. **(b) A separate table** (the proposal):
object → region or overlay group, a side table keyed by object id (91);
recomputing is producing another table. **(c) Leave it to the linker
script**, as nearly every embedded system does: the compiler does not know
where things are and cannot compute the WCET or iterate. Ruled out for the
processor.

On x86 and ARM (b) is trivial: the table puts everything in `.text`,
`.rodata`, `.data`, `.bss` and the linker chooses addresses. One mechanism
for the three targets; only the target's region table changes.

**Details.**
1. **What is an object**: functions, globals, literals, vtables, stacks (one
   per hardware thread). **Local slots are not**: they are positions in a
   frame inside the stack object, even when their address escapes. A heap,
   if any (42, open), is a region, not an object.
2. **Objects born below the IR join the same table**: constant pools for
   64-bit constants (30), a `switch`'s jump tables. Born in selection,
   placed by the same mechanism.
3. **Each object carries its contents**: initial bytes (made by the layout
   in the target's endianness) or "zeroed" (`.bss`); an object may be
   **read-only**, which allows ROM or flash.
4. **Placement is per whole object**; to place part of a function it is
   split first (function splitting, in the structured form) and the part
   becomes an object.
5. **Regions belong to the target description** (base, size, latency, code
   or data, private or shared, reachable by DMA); **the placement table
   belongs to the program**, since compilation is whole-program (89).

**A tension with 35 (open).** If the ISA reaches every memory with the
**same** loads and stores, re-placing changes only addresses and costs,
selection is not redone, and (b) works exactly as described. If there are
**different instructions per memory** (as Patmos), moving data changes the
instruction of whoever accesses it: data placement must come before
selection, or selection is redone per iteration (code placement is
unaffected). The IR's shape is the same (the address space in the pointer,
80); only the order of the processor back end's passes changes.

**Recommendation: (b)**, with details 1-5, the dependency on 35 recorded.

**Answer: (b).** On 35, Hadley: probably the same instructions, not
certain yet.

## 82. Locals: value or slot

**The question.** An IR local lives either as a **value** (an IR name
defined once, in a register) or in a **slot** (a piece of the stack frame,
read by `load` and written by `store`). The proposal: a **scalar** never
**rewritten** after its definition and whose **address is never taken** is
a value; everything else gets a slot; mem2reg promotes later.

**(a) Everything is a slot** (clang: `alloca` for every variable, mem2reg
cleans up). The simplest lowering; until mem2reg the IR is heavy (every use
a `load`) and the code poor. **(b) The proposal**: most Haard `let`s are
written once (`let n = xs.size()`) and become values for free; the front end
knows everything needed (the ResolutionTable links each use to its
declaration). **(c) SSA directly in the lowering** (Braun et al. 2013,
Cranelift's front end): even rewritten variables become values, no slots or
mem2reg for scalars; but in the structured form every region must carry the
values that change in it (`if` results, loop-carried arguments as MLIR's
`scf.for` `iter_args`), more complex exactly in the form the lowering
produces.

**"Address taken" in Haard** is more than `&x`: passing `x` to a `T&` or
`T&&` parameter (`&&x` included), `let r : T& = x`, **capture by a closure**
(by reference, record 0058), and a method call on `x` (`this` is the
address; only aggregates, already slots by 80). A **parameter** follows the
same rule, copied into a slot at entry when needed.

**Found while checking `for`.** `for i in 0..n` does not reach the IR as a
counted loop: the ForEachLowerer (record 0040) rewrites it first into
`let __e = n; let i = 0; for ; i < __e ; i = i + 1:`. Under (b) `i` would be
a slot and the loop a generic `loop`, losing induction, step and count. The
proposal was for the lowering to recognise the canonical counted loop in the
C-style `for` (step `i = i + c` with constant `c`, condition `i < e` or
`i <= e` with `e` not written in the body, `i` not written or
address-taken in the body) and make a `for` region with `i` as its
argument, any other loop a `loop` with a slot.

**And a real bug**, tested: `let last : u8 = 255; for i in 0..last:` never
ended. `i <= 255` is always true for a `u8`; with wrapping (1a) `i` goes
from 255 to 0 and the loop never ends (the classic C inclusive-`for` bug).
The inclusive range must test **before** stepping, or count as
`e - a + 1` in a wider type.

The recommendation was (b), with the counted-loop recognition, the analysis
as a pre-pass over each body in the lowering, and mem2reg later in the CFG
form (a promotion on the structured form when vectorisation comes).

**Answer: (a), every local a slot**; mem2reg later; an inefficient IR is
fine in the first versions. The counted-loop recognition waits for loop
passes. The bug was fixed the same day: the inclusive range asks whether
there is another turn before stepping, and steps at the top of the next
turn (record 0040, amended;
`tests/emitter/cases/an_inclusive_range_reaches_the_top_of_its_type`).
While writing that case, `let low : i8 = -128` was refused with *128 does
not fit in i8*: the literal is checked before the minus is applied (added to
the small pending items).

## 83. Virtual calls, destructors and temporaries

**Virtual calls.** Every method of a class is virtual (record 0020), a
struct's never is (0065). A virtual call can reach the IR **already
expanded** (LLVM, what C++ produces: load the vptr, load the vtable entry,
call indirect) or as **its own instruction**, `%r = vcall Shape.area(%s)`,
expanded only by instruction selection (Swift SIL's `class_method`, the
JVM's `invokevirtual`). The instruction makes devirtualisation a trivial
rewrite (`vcall` becomes `call` when the dynamic class is known, where the
expanded form needs a proof about memory), gives the WCET the possible
targets (the class table lists every override, the program being compiled
whole, 89; an indirect call through a pointer cannot satisfy the real-time
rule on indirect calls, 61), and keeps `--emit-ir` readable. The cost is
one expansion in selection; the interpreter dispatches through the vtable
table it already has for the layout.

Two rules are semantics, not optimisation: a receiver held **by value**
calls directly (its dynamic class is its static one), and so does
`super.method()` (record 0074). The third is the vptr during construction
and destruction. Under the **C++ rule** (the current behaviour, since
record 0026 emits `init`/`destroy` as C++ constructor and destructor) the
object **is** its base while the base's `init` runs, so a virtual call made
there reaches the base's version; the derived vptr is stored once the base
is done, and destruction runs the other way. The **Java/C# rule** stores
the most derived vptr from the start, letting a base call an override that
reads fields of the derived class that have no value yet (record 0026 leaves
a field without an initialiser without a value), and the oracle would stop
matching the C++ path. Construction: build the base (recursively), store
the own vptr, initialise the fields with a written value, run the `init`
body. Destruction: store the own vptr, run `destroy`, destroy the fields,
destroy the base.

**Cleanups.** The IR has no implicit end of scope: every `destroy` is a
`call` written by the lowering, placed by a **cleanup stack** (clang's
`EHScopeStack`): each scope pushes what must run when it is left, and every
exit emits the calls innermost first. Much simpler than clang's because
there are no exceptions (4a): no landing pads, no unwinding, every exit is
visible (`return`, `break`, `continue`, the end of a block). The other
option, regions carrying an "on exit" attachment expanded later, would make
every pass understand the attachment.

The rules, those the C++ path already follows: reverse order of
declaration; a `return` emits the cleanups of every scope it leaves, a
`break`/`continue` those inside the loop; the return value is written to
its destination **before** the cleanups. Temporaries (record 0068) die at
the end of the statement, one bound to a `T&` at the end of the block. A
temporary made on one path only (`f() or g().ok()`) gets a **flag slot**,
false at first, set when it is made, and its destructor runs only when the
flag is set (clang's technique; with every local a slot, 82, it costs a
store and a test). A moved-from object (`&&x`) is still destroyed. `new
T(args)` is `call @__alloc(size T)` plus the construction; `delete p` is
`vcall destroy` plus `call @__free`, runtime intrinsics whose meaning on
the processor waits on 42.

Two details were added to the recommendation: `delete` of a null pointer
does nothing (C++); and an aggregate crossing a call (a consequence of 80)
is copied by the **caller** and destroyed by the caller at the end of the
statement (the Itanium ABI rule), while a result by value is constructed by
the callee **straight into the destination** pointer (C++17's guaranteed
elision), so `let s = make()` calls no copy constructor.

**Answer: approved** with every rule above: `vcall` until selection; direct
calls for a by-value receiver and `super`; the C++ vptr rule; explicit
cleanups through a stack, in reverse order, the return value written first;
temporaries per record 0068; a flag for a conditional temporary; a moved-from
object still destroyed; `new`/`delete` through `__alloc`/`__free`, `delete`
of null doing nothing; by-value arguments copied and destroyed by the
caller; results constructed in the destination.

## 84. Loop bounds (flow facts) in the IR

**The question.** Static WCET has three steps: flow analysis (how many
times each loop runs), microarchitectural analysis (cycles per block) and
the worst path (an integer linear program). On a PRET processor the second
is simple; the hard one is the first, and the compiler knows the answer:
`for i in 0...64` states its count. Tools reading the binary (aiT, OTAWA)
lose it and ask for hand annotations. The question is how each loop's bound
lives in the IR, and who keeps it right after every pass.

**Elsewhere.** LLVM has loop metadata (`llvm.loop`) that any pass may drop
silently, the known problem for WCET built on it. platin (T-CREST, Patmos)
and WCC (TU Dortmund) carry flow facts from the source to the machine code,
and every optimisation must update them; WCC reads a `loopbound min 0 max
10` pragma in C. aiT reads annotations from a separate file (`loop "f" + 1
loop max 10`), written by hand when it cannot infer.

**The options.** (a) Optional metadata in a side table, as LLVM: a pass
that does not know it loses it. (b) A mandatory field of the loop: on the
`loop`/`for` region and, after flatten, on the loop header (78); only a
pass that rewrites a loop's bound may change that loop (unroll by k turns N
into ⌈N/k⌉, vectorise by 4 gives ⌈N/4⌉ plus an epilogue of at most 3, peel
gives N-1); the verifier (88) requires it on every loop of real-time code.
(c) Nothing in the IR, the bound recomputed on machine code: the failure of
the binary tools again.

**Details of (b).** (1) A constant maximum per entry into the loop;
symbolic bounds ("at most `n`") lead to parametric WCET, research for now.
(2) Relative facts later: in a triangular nest (`for j in 0...i` inside
`for i in 0...n`) the inner loop is bounded by n per entry and the total
comes out n² where the truth is n²/2, pessimistic but safe, and a total
bound can be added without changing anything. (3) The interpreter checks
every bound on every run and fails the test when a loop goes past it,
catching a wrong annotation or a pass that updated it wrongly: a second
oracle for free. (4) A field of the region, not a side table, on purpose,
an exception to 91: a field cannot be forgotten by whatever copies or
rewrites the loop. (5) The IR does not depend on 57 (where the real-time
restrictions apply) or 58 (bounds inferred or annotated, and the spelling):
it holds an optional maximum per loop; the verifier decides when it is
required.

**Answer: approved, (b)** with the five details.

## 85. What a barrier is

**The question.** Three kinds of instruction cannot be treated as ordinary
arithmetic by the optimiser: **timing instructions** (`get_time`,
`delay_until`; which exist is 64), **volatile accesses** (`volatile_read`,
`volatile_write`, `fence`; 11 already made them full barriers) and
**memory transfers** (copying an object into a scratchpad, starting a DMA
and waiting for it). The proposal calls them barriers; the question is what
exactly a pass may not do with them and with the code around them.

**Elsewhere.** In C/C++ and LLVM a `volatile` access is ordered only against
other volatiles; ordinary loads and stores may cross it, so writing a
buffer and then the register that starts a DMA needs an explicit `fence`,
a classic bug. Linux's `writel` embeds the barrier to avoid it. LLVM and
MLIR have every opcode declare its effects (reads memory, writes memory,
has side effects) in one place all passes ask (`mayReadFromMemory`,
`MemoryEffects`). WCET-aware compilers (WCC, platin) move no code across
timing instructions, since the interval between two time marks is what is
being measured.

**The options.** (a) A full barrier: no instruction crosses the three
classes, in either direction; simple and pessimistic. (b) Fine effect
classes as in C: contradicts 11 and brings the DMA bug back. (c) (a),
written as a table of effects per opcode, with explicit rules.

**The rules of (c).** (1) Nothing crosses, not even pure arithmetic;
between barriers the optimiser is free. (2) Never removed, even when the
result is unused. (3) The dynamic count and the order among them are kept:
unrolling copies the text, but each execution still happens once and in the
same order. (4) Never speculated: an if-conversion that runs both sides
(`select`) may not include one; real predication, where the instruction
does not execute, may. (5) Visible to the WCET, each with its cost in the
timing table. (6) One table: every opcode declares its effects (reads,
writes, barrier) and every pass asks it, never a list of its own. (7) In
the first version a call is a full barrier too; with whole-program
compilation (89), per-function effect summaries ("touches no memory",
"contains no timing instruction") can loosen this later without changing
the IR. There is no optimiser yet, so it costs nothing today.

**Answer: approved, (c)** with the seven rules.

## 86. Source positions and the machine ↔ IR ↔ source mapping

**The question.** Does each IR instruction know where in the source it came
from, and each machine instruction which IR instruction it came from? This
feeds the WCET report with the worst path in source lines (68), DWARF later
(76), the messages of late passes ("this loop has no bound", 84) and
debugging the compiler itself.

**Elsewhere.** LLVM gives every instruction a `DILocation` (line, column,
scope, and an `inlinedAt` for inlined code); passes should keep it, CSE
merging two instructions of different lines sets line 0, and in practice the
information is often lost, a chronic source of debugging bugs. Cranelift
keeps a `SourceLoc` per instruction in a side table, and its machine code
knows the IR instruction it came from. Go and Zig have a position per
instruction from day one. Small compilers (QBE, chibicc) keep a line per
statement, or nothing.

**The options.** (a) Nothing until DWARF: expensive to add later, every pass
already written would have to be revisited. (b) A position per instruction
now, with a machine → IR mapping (the proposal). (c) A position per
statement or block: too coarse for the WCET's worst path and for DWARF.

**The details of (b).** (1) A position is `(module, byte offset)`, as tokens
and the logger already have it; line and column are derived only when
shown. An instruction gets the token of the Ast node that produced it: the
operator of `a + b`, the name of a call. (2) It lives in a side table keyed
by instruction id (91). So it is not forgotten, the IR builder takes the
position as an argument of every creation, and the verifier (88) checks
that no instruction lacks one. A field of the record, as in 84, is not
needed, since a position never changes meaning in a pass: it is only
copied. (3) An instruction the compiler makes (83's cleanups, a vptr store,
a flag slot, an argument copy) gets the position of the construct
responsible: the `return` or `break` that runs a cleanup, the declaration
for an end-of-block cleanup, the `init` for a vptr store. The WCET report
then says "destructor of `buf`, exit at line 12", never "line 0". (4) A
pass merging two instructions keeps one of the positions, never none: an
approximate line is worth more to the WCET than no line. (5) Inlining: an
optional `inlined_at` (the id of the call the instruction came from), in a
table of inline sites, reserved and always empty until an inliner exists.
(6) Machine → IR: every machine instruction keeps the id of the IR
instruction that made it, one IR instruction possibly making several;
prologue, epilogue and spills point to the function's entry or to the
instruction that caused the spill. (7) Types for a debugger (left by 79):
every local being a slot (82), each slot keeps the id of its declaration,
which gives the name and the Haard type. mem2reg will need an equivalent of
LLVM's `dbg.value`, when it comes.

**Answer: approved, (b)** with the seven details.

## 87. The IR interpreter

**The question.** 78 decided that the `IrInterpreter` runs only the CFG
form. It is the oracle: every program of the emitter suites goes through
both paths (C++ and IR) and must give the same exit status. Left to decide:
how it represents memory, and which checking modes it has, which together
set how many bugs it can catch.

**Elsewhere.** Flat memory (ISA simulators, QEMU, LLVM's `lli`): a byte
vector with real addresses, faithful to the machine, but an access outside
an object silently reads its neighbour. Memory by object (Rust's Miri,
CompCert, the Cerberus interpreter for C): each object is a separate block
and a pointer is `(object, offset)`; an access outside the block, or to a
freed one, stops the program with a diagnostic. To allow pointer ↔ integer,
Miri gives each object a fictitious base address and arithmetic still
works.

**The options.** (a) Flat memory with the addresses of the layout and the
placement. (b) Memory by object, with a base address per object for
conversions to integers.

**The details of (b).** (1) The objects are 81's (functions, globals,
literals, vtables), plus a block per slot of every stack frame and one per
heap allocation; bytes in the target's endianness, by the layout (80). (2)
It stops with a diagnostic and a source position (86) on an access outside
an object, a use after `__free` or after the frame is left, a double
`__free`, and a touch of an overlaid object that is not resident. Nearly
every lowering mistake lands in one of these; the residency check already
needed object identity, which (b) gives for free. (3) MMIO: a range of the
memory map may be declared a device, and a `volatile_read`/`volatile_write`
there calls a model of it; at first the only devices are the natives of
`std.low_io`. (4) Parameters: the target description (endianness, pointer
width, layout), the memory map with regions and latencies, the timing
table. (5) Checking modes, all on by default in the tests, with a fixed
seed so a result is reproducible: garbage upper bits (the 2026-10-08
decision that upper bits are undefined: a missing extension changes the
exit status); new memory filled with a garbage pattern (10); loop bounds
checked (84); cycle counting with the timing table, the test checking
measured ≤ WCET (69), on only when the target is the processor. (6) Aborts
(9b's bounds check, `unwrap`) exit with the same status as the C++ path, so
the oracle stays comparable. (7) One hardware thread for now; several come
with 63, still open. (8) Simple: a loop with a `switch` over the opcode, no
JIT, no tricks; speed matters only if the suites get slow.

**Answer: approved, (b)** with the eight details.

## 88. The verifier

**The question.** The verifier checks that the IR is well formed (whether
the program is right is the interpreter's job). The proposal runs it after
the lowering and after every pass, always in the tests. Left to decide:
what it checks, when it runs outside the tests, and what happens when it
fails.

**Elsewhere.** LLVM's `Verifier` runs after the front end; after every pass
(`-verify-each`) is optional, expensive with hundreds of passes. Cranelift's
verifier is on by default, in production too, its cost judged small next
to its value. MLIR has every operation declare its invariants, verified
between passes by default. A failure is always a compiler bug, never the
user's: all of them stop with an internal error and the function's dump.

**The options.** (a) Only in tests and debug builds. (b) Always, in
production too, with a flag to turn it off. (c) Only after the lowering.

**Recommended (b)**: its cost is linear in the IR, and the 2026-09-09
measurement (500k lines in 37 s) leaves room; if it weighs, it is turned
off after measuring, never out of the suites. What it checks: (1) form
(78): one form per function; in the structured form regions well nested
and `break`/`continue` aiming at an enclosing region; in the CFG every
block ending in exactly one terminator, targets existing, the entry with no
predecessors. (2) Types: operands matching the opcode (`add b32` takes two
`b32`), 8 and 16 bits only in memory (2026-10-08), the fields and elements
of `field`/`index` existing in the layout table (80). (3) Every use
dominated by its definition. (4) Calls: arity and types matching the
signature, a `call`'s target existing, a `vcall` naming a method of its
class (83). (5) Every referenced object in 81's table, every slot belonging
to its own function. (6) Side tables: every instruction with a position
(86), every loop of real-time code with a bound (84; what counts as
real-time code is 57). (7) The effects table (85) with an entry for every
opcode, checked once in a test. (8) A failure: an internal error naming the
pass that just ran, the function's dump with the culprit instruction
marked, then abort. (9) Its suite: IR broken on purpose, one case per rule,
built with the `IrBuilder` in C++ (the text is only a dump, so there is no
IR parser), each case proven by sabotage.

**Answer: approved, (b)** with the nine details.

## 89. Whole-program compilation

**The question.** Does hdc compile the whole program at once, into one IR
and one object, or module by module, joined at link time?

**What Haard already does**, without having decided it: dependencies are
source, not binaries (records 0010, 0044); generics are monomorphised
(0002), an instantiation existing only where something uses it; the C++
emitter writes the whole program in one file (`emitter.h`); the 2026-09-09
measurement gave 500k lines in 37 s with no cache.

**What depends on it in the IR**: the WCET needs the complete call graph
and every target of indirect calls (61, 83); the worst stack use is a sum
over that graph; overlays need to know what is never live together (81);
devirtualising a `vcall` needs to know no other override exists. None of
it works with separate compilation.

**Elsewhere.** C and C++ compile separately, the whole program only through
LTO, optional and slow. Rust's unit is the crate, LTO optional. MLton
(SML), Zig, Go, GraalVM native-image and the WCET compilers (WCC, platin)
compile the whole program; Zig and Go compile everything from source on
purpose.

**The options.** (a) Whole program always, on every target. (b) Module by
module, a link phase rebuilding the graph. (c) Whole program only on the
processor.

**Recommended (a)**: it is already the model; (b) would redo LLVM's LTO,
(c) would mean two pipelines. Details: (1) one IR per program, the tables
of 80 and 81 global, one object per back end, given to the system's `ld` on
x86/ARM (71). (2) Only what is reachable goes in: roots are `main` and
exported functions, following calls, referenced objects and the vtables of
classes someone constructs; this matters to fit the scratchpads. Dropping
methods of classes never constructed (Rapid Type Analysis) comes later.
(3) External C functions (74) are opaque: calling one is a full barrier
(85); it enters the WCET only with an annotated cost, so real-time code
calling one needs the annotation (a record of its own); it may not call
back into Haard through a pointer without an annotation (61's rule). (4) No
Haard dynamic libraries for now; a Haard program may later export functions
with the C ABI, as extra roots. (5) Incremental recompilation stays with
record 0015: a cache, if it comes, keeps the front end's result per module;
the IR and the back end always run whole, decided after measuring if it
gets slow.

**Answer: approved, (a)** with the five details.

## 90. The WCET computed by hdc

**The question.** The proposal has hdc compute the WCET itself on the
processor's final machine code. The question is how: the classic
worst-path technique, IPET, is an integer linear program and needs an ILP
solver, and the rule against external dependencies forbids taking one
(lp_solve, CPLEX).

**Elsewhere.** IPET (aiT, OTAWA, platin): a variable per block counting its
executions, constraints for flow and loop bounds, maximise the sum of
costs; the industry standard, handling any CFG, relative bounds and
infeasible paths ("through A, then not through B"). The timing schema, or
tree-based analysis (Shaw 1989, Puschner and Koza), walks the program's
structure: a sequence is a sum, an `if` the maximum of its sides plus the
condition, a loop bound × (body + test) + the final test, a call the
callee's WCET. It needs structured code, which is why it fell out of use
for C with `goto`, and it cannot express relative bounds.

**What changed in Haard.** With `goto` removed (0078) every Haard program is
structured, and flatten makes a reducible CFG with a bound on each loop
header (78, 84). The tree-based analysis holds for everything, and on a
PRET machine with fixed latencies it is exact per block.

**The options.** (a) Tree-based first, IPET later with a solver of our own
when relative bounds (84, detail 2) or infeasible paths come. (b) IPET now,
with our own solver (simplex plus branch and bound): more code up front, a
gain only with facts that do not exist yet. (c) An external tool (OTAWA):
breaks the dependency rule; possible later as an optional export.

**Recommended (a)**, with: (1) input: the final machine code after placement
and overlays (81), with the machine → IR → source mapping (86), the bounds
on the headers (84), the timing table (62, open) and each memory region's
latency, the cost of overlay loads and of barriers and transfers (85). (2)
The loop nesting is rebuilt from the reducible CFG, no extra markers
needed. (3) Calls bottom-up over the call graph (89); a `vcall` costs the
maximum over the class table's targets (83); a recursive function, or an
external C function without an annotated cost, is unbounded. (4) Unbounded
is not an error by itself: the report says "unbounded" with the reason and
the position (loop without bound, recursion, C call); it is an error only
in real-time code (57). (5) Outputs: WCET per function; the worst path in
source lines (68), which in a tree-based analysis is recording which side
of each `if` gave the maximum; the worst stack use (55), a sum along the
graph. (6) In every test on the processor, the cycles the interpreter
measures are ≤ WCET (69, 87). (7) Interleaved threads (23): the
interleaving being fixed, each thread sees fixed latencies, given per
thread by the timing table; hardware threads sharing memory come with 63.
(8) No WCET on x86/ARM.

**Answer: approved, (a)** with the eight details.

## 91. How the IR is stored

**The question.** Is the IR a graph of pointers between objects (LLVM: each
`Instruction` a heap object, linked by pointers, with use lists) or plain
data indexed by id (Cranelift, Zig)? If by id: where the data lives, how
order is kept, how ids survive edits.

**Elsewhere.** LLVM and GCC: a pointer graph with use lists; replacing every
use of a value is easy, but each instruction is an allocation and a side
table becomes a hash map. Cranelift: arenas per function, 4-byte ids, order
inside a block as an index-linked list, an alias table instead of use
lists. Zig (ZIR/AIR) and Carbon: contiguous vectors and indices, chosen for
speed. Haard's Ast is already so, and the rule "never hold a pointer into a
vector across an insertion" already cost bugs there.

**The options.** (a) Plain data by id, stored per function (the proposal).
(b) A pointer graph as LLVM: natural in C++, hard to rewrite one to one in
Haard, side tables expensive. (c) One global arena for the whole program:
cloning, removing or processing one function alone gets hard.

**Recommended (a)**, with: (1) per function, vectors of instructions,
blocks, slots and values (an instruction's result and the parameters);
4-byte ids, each kind in a struct of its own (`InstId`, `BlockId`,
`SlotId`) so the C++ compiler refuses mixing them, plain C++ with no
templates per the bootstrap rule. (2) Fixed-size instructions: opcode, type
and up to three operands inline; calls, `switch` and regions keep their
operand list in the function's operand pool as (start, count). (3) Order
inside a block is a doubly linked list by index (previous/next): insertion
and removal in O(1) without moving anything. (4) In the structured form a
region (`if`, `loop`, `for`, `block`) is an instruction owning child
blocks, kept in the pool; flatten rewrites them into CFG blocks. (5) What
belongs to the program (80's types, layout and classes, 81's objects and
placement, 86's inline sites) is global, by id too. (6) Side tables
(positions, analysis results, the machine mapping) are parallel vectors
keyed by id; the decided exception is the loop bound, a field (84). (7) Ids
are stable inside a pass: removal only marks; compaction runs between
passes and returns a remap. One compaction function knows every side table
of a function, registered in one place, so none is forgotten; the verifier
(88) checks they all have the right size. (8) No use lists: "replace every
use" goes through an alias table, resolved on read and emptied at
compaction. (9) Access always by id, never by a pointer or reference held
into a vector: the Ast's lesson.

**Answer: approved, (a)** with the nine details. Block G (78-91) is
complete.
