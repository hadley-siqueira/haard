# 0084 — The native back ends

Status: **decided**, 2026-11-04, with the spelling of a label's root still open
(below). Hadley answered questions 71-77 of `notes/ir-questions.md`, then ten more
questions one at a time; the table is his. **Steps 1 and 2 of the order below are built, in
`toolchain/`, 2026-11-05, and step 3 is under way.** The reasoning behind the first seven is in
`notes/ir-questions-explained.md`.

| | |
|---|---|
| An assembler and a linker of our own, **written in Haard**, the first version going through the C++ path | **decided**, 71 |
| `as` and `ld` are not a supported route; `as` may be an **oracle in the test suite**, and nothing else | **decided**, 71 |
| The processor has no assembler; this one will be it, in Haard | **decided**, 72 |
| Linux only in the first version | **decided**, 73 |
| No interoperating with C, unless it becomes extremely necessary | **decided**, 74 |
| Direct system calls, no libc | **decided**, 75 |
| Debug information is generated, and the tools may work closely with the compiler; no `as`/`ld` legacy to keep | **decided**, 76 |
| A fixed address first, with the code already position independent; **PIE the default on Linux before a public version** | **decided**, 77, option (c) |
| **x86-64 is the first target**: every step runs here at once, where ARM64 needs an emulator in the suite and the processor's ISA is still moving | **decided**, Hadley, 2026-11-04 |
| **Two steps for now**: `hdc` writes text and a `Makefile` runs the tool, as with g++ today. When `hdc` is in Haard, the tool goes inside it, **if that still makes sense then**. A separate driver, as `go build`, can come later without touching either | **decided**, Hadley, 2026-11-04 |
| **The text is Intel order, one instruction per line, a subset `as` reads** given `.intel_syntax noprefix`. What is only ours is a directive of ours, stripped for `as` in the tests. This is no promise of compatibility: it may diverge if the oracle stops being useful | **decided**, Hadley, 2026-11-04 |
| **A branch on x86-64 always takes the long form**, four bytes of distance, with no relaxation: every size is known in one pass, which WCET wants. Measured on `hdc` built by g++ with no `-O`: 3.6% more code. `as` chooses the short form by itself, so the tests' filter adds `{disp32}`, which was tried and forces the long one. Choosing the short form can be added later | **decided**, Hadley, 2026-11-04 |
| **A conditional branch on ARM64 follows a rule per function**: up to 1 MB, always `b.cond`; larger, always the inverted branch over a `b`. Every instruction is 4 bytes, so counting them gives the size in one pass, with no relaxation. To be reviewed if ARM64 shows otherwise. The largest of `hdc`'s 8,065 functions on x86-64 is 17,077 bytes | **decided**, Hadley, 2026-11-04 |
| **The calling convention is our own**, free of the C ABI, in System V's register order: whole numbers and pointers in `rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9`, floats in `xmm0`-`xmm7`, the rest on the stack, the result in `rax` or `xmm0`. A struct or class by value always goes through a hidden pointer, the result too. No varargs and no red zone. Only `rsp` and `rbp` are preserved for now, and `rbp` is always the frame pointer. A program compiled whole may change this at any time | **decided**, Hadley, 2026-11-04 |
| **No register allocation at first**: every value in its slot, as the IR's memory form has them (record 0077). **A global allocator, over SSA and linear scan, once `tests/emitter` passes natively**, and the plain path stays to compare against. Nothing per block in between | **decided**, Hadley, 2026-11-04 |
| **Sizes and addresses are in the ELF symbol table now**, with each label's name, address and size, which gdb, `readelf` and `objdump` read. **A map only when WCET needs one**, and it may never be needed once the tool lives inside `hdc`. `hdc` does not compute sizes itself, which would duplicate the encoder | **decided**, Hadley, 2026-11-04 |
| **Debug information is DWARF, written by our tools, in parts**: `.debug_line` early, since it helps debug the back end itself, reaching the tool through a directive of ours; `.debug_info` later. The symbol table comes first (above). **A format and a debugger of our own would be interesting in the future**, since they could know everything about Haard, enums and generics included | **decided**, Hadley, 2026-11-04 |
| **`floor` is an IR instruction, `ffloor`** (`roundsd`, which needs SSE4.1, on x86-64, `frintm` on ARM64). Its result is exact, so no implementation can disagree | **decided**, Hadley, 2026-11-04 |
| **`pow` is written in software, in Haard, in `std`, of good quality**: an algorithm such as fdlibm's, using the machine's ordinary float operations, the same on every target and on all three paths. A crude `exp(y * log(x))` measured a worst of 102 ulp against libm and 7.3 on average, and would change programs' results when improved. It may come last in the back end; until then the C++ path keeps libm's | **decided**, Hadley, 2026-11-04 |
| **The first goal of the back end is a correct result, not speed.** If need be, the worst case can go entirely to software, with no help from the hardware | **decided**, Hadley, 2026-11-04: *"o objetivo inicial é garantir que o backend produza resultado correto e não velocidade de execução"* |
| Labels keep their readable names, `"std.io.print(i32)"`, natively, with parallel builds to `.o` in mind | **asked**, Hadley; two points are open, under *Names* |

## The shape

```
hdc  -->  text  -->  assembler and linker, in Haard  -->  a static ELF
```

`hdc` writes text and a separate tool encodes it. This is what makes it a
simplification: the compiler keeps the small part (about 300 to 400 lines per target,
an estimate), and x86-64, ARM64 and the processor share one way out. When `hdc` itself
is in Haard, the tool can become a library inside it, with no process and no text.

## The bootstrap

1. `hasm.hd` through `hdc`, C++ and g++ gives a working assembler. **Measured**, see below.
2. With a native back end, `hdc` compiles `hasm.hd` to text, and that assembler makes a
   native one.
3. The native one assembles itself, and the result has to be identical byte for byte.
4. When `hdc` is in Haard, the tool can move inside it.

Steps 2 to 4 are not proven; they wait on the back end. The C++ path is what lets the
whole chain be rebuilt from nothing, with no binary kept in the repository, so **it
stays**, as the bootstrap and as an oracle.

## What was measured

A probe of 287 lines of Haard, 2026-11-04, not kept in the repository: a few x86-64
forms, quoted labels and a static ELF.

- Through Haard, C++ and g++, it built a program that ran with no `as`, `ld` or libc.
- On a generated file of a million lines, its bytes were identical to `as`'s, in 0.64 s
  and 40 MB against 0.97 s and 20 MB. It knows far fewer forms than `as`, so this shows
  that Haard is not too slow for the job, not that it is faster.
- `mov edi, eax`, a form it did not know, became garbage in silence. The million lines
  did not have that form, and a small program found it. So **the real one refuses
  whatever it does not know**, and being an oracle is what `as` is for.
- `ld` took `"std.io.print(i32)"` with nothing mangled. Mangling never was the linker's
  demand beyond a unique name.
- One program of 138 bytes, the same instructions, read the same address on six runs
  at a fixed address and six different ones as PIE.

## Names

The label is the full name, readable, in the text and in the symbol table. A program
compiled whole needs nothing more. Parallel builds to `.o` would need two more things,
and it is cheaper to leave room for them now than to rename later:

- **The root or version**: record 0010 allows two versions of one library in one
  program, so `"std.io.print(i32)"` alone is not unique. Something like
  `"std@1.2:std.io.print(i32)"`, spelling open.
- **A generic instantiated twice**: two `.o` may both make `Array<i32>`. The linker
  needs a rule, keeping one, and in a format of our own that can be part of the format.

The `.o` format and the full linker come with parallel builds. The first version writes
only a static ELF.

## What has to be written that libc gave

- **`_start`**: reads `argc` and `argv` from the stack, runs what comes before `main`,
  and exits through the system call.
- **An allocator** behind `__alloc` and `__free`, over `mmap`. Its behaviour matters to
  the WCET report and has to fail with a message.
- **The natives of `std.low_io`**: `__io_open_read`, `__io_open_write`, `__io_read`,
  `__io_at_end`, `__io_write`, `__io_close`, `__io_flush`, `__io_stdout`, `__io_stdin`,
  `__io_stderr` and `__abort`. Today they lean on C's `FILE`, whose **buffer** is what
  makes one character at a time cheap. A system call per character would not be, so the
  buffer has to be written, and flushed at exit and before stopping.
- **`__check_index`** is no native here: the IR already has it as an instruction.
- **Making a file executable**, a native Haard does not have and the tool needs. The
  probe used `chmod` from the shell.

## What stays as it is

- **The layout** still follows the Itanium ABI (record 0077), although no C or C++ is
  called. `tests/ir_layout` checks it against g++, and the C++ path needs it while it
  is an oracle. Whether to leave it, later, is its own question.
- **Questions 25-70** concern the processor and do not block x86-64.

## What step 1 built

`toolchain/hasm/`, a project of its own: `hasm <input.s> -o <output>` makes a
static ELF. `toolchain/hasm/README.md` lists what it reads. Its tests hold three things against
`as` and `ld`: every form it knows, byte for byte; every instruction against every shape
of operand, where whatever it does not refuse has to come out the same; and the
addresses, sections, symbols and entry.

The choices the table above left open, each easy to change:

- **Every symbol is `STB_LOCAL`.** A program is compiled whole, so nothing is left for a
  global to mean. `.globl` is refused, and the tests' filter adds it for `ld`.
- **`.balign` is refused in `.text`**, where `as` fills with its own table of long nops.
  Nothing needs a function aligned while the goal is a correct result.
- **A memory operand has one spelling**, `[base + index*scale + disp]` or `[rip + label]`,
  with no offset after a label. An absolute address is refused.
- **`.ascii` knows `\n`, `\t`, `\\` and `\"`.** Other bytes are `.byte`, because `as` reads
  `\x` with as many digits as follow, where record 0082 reads two.
- **The output is not marked executable**: Haard has no way to yet (below), so the caller
  runs `chmod +x`.
- **Not yet there, added when the back end asks**: `rep movs`, and the memory form of the
  SSE bitwise operations.

## What step 2 built

`toolchain/runtime/x86_64_linux.s`, in assembly, which `hasm` takes with what `hdc` writes,
since it now reads several files as one program. `toolchain/runtime/README.md` says what it
gives and what a program has to define. What the back end has to know:

- **`hdc` writes `__before_main` and `__after_main`**, which call the IR's initialisers and
  finalisers, and `main` takes the count and the table of arguments, as the IR's does.
- **`__abort_with(message)`** is how an instruction that stops says why. The message of a
  failed `check_index` needs a number written, and comes with step 3.
- **`__pow` is not there**, as the table above allows; `__floor` is, until `ffloor`.

Output follows C's stdio, the C++ path being its oracle. The one difference is held on
purpose and cannot be told from outside but in time: `stderr` is held to the end of a line,
or until anything else would go out or wait. A byte at a time, as C's, made a system call a
byte, and `hasm` took 4.5 s of system time to report 240,000 refusals that way.

**The runtime moves to Haard after step 3** (Hadley, 2026-11-05: *"depois vamos fazer sim a
reescrita para haard o que for possível e manter em assembly apenas o essencial, como o
syscall"*). The allocator and the files become Haard over one `__syscall` native, leaving
`_start`, that native and the system call numbers per target; ARM64 shares them, and the
processor the allocator's logic, its start-up and any OS being open in questions 32-45. Not
before: while the back end can still be wrong, a runtime independent of it says whose bug
it is. The allocator cannot use `new`, and nothing there may depend on a global built before
`main`.

## How step 3 goes

`src/haard/backend/x86_64_writer.cpp`, by `hdc --emit-asm`, judged by `tests/native`. The order,
each with programs running:

1. **Built.** Whole numbers, locals, `if`, loops, calls.
2. Memory: slots, fields, indexes, structs by value, `copy`.
3. Globals, text, `new` and `delete`: the first `println`. Finalisers run last built first,
   which nothing can check before this.
4. Classes, vtables, enums, tuples, closures, function values.
5. Floats and conversions.
6. All of `tests/emitter` and `tests/programs`.

**Every value lives in eight bytes of the frame, zero extended from its width**, as the
interpreter holds it, and an instruction works in 64 bits and cuts the result to its width.
So the rules of each width are the interpreter's, and x86-64's partial registers never come
up. An instruction no program can reach yet is refused by name, not written and left unproven:
today `smulhi`, `umulhi`, `inttoptr`, `ptrtoint` and a block taking arguments.

## The order

1. **Built.** The assembler in Haard for x86-64, refusing what it does not know, with `as`
   as the oracle in its tests.
2. **Built.** The natives and `_start`, with a small program running with no C.
3. **Under way, part 1 of 6.** The back end in `hdc`, running `tests/emitter` natively as a
   third path beside C++ and the IR, which are its oracle.
4. The bootstrap's steps 2 and 3.
5. ARM64, then the processor, through the same tool.
6. Debug information, then PIE as the default before a public version.
