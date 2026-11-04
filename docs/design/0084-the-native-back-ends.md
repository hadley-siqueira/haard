# 0084 — The native back ends

Status: **decided in part**, 2026-11-04. Hadley answered questions 71-77 of
`notes/ir-questions.md`; the first table is his. The second is **proposed and not
decided**, each row waiting on him. **Nothing here is built**: he asked for it to be
written down first (*"não implemente nada ainda por enquanto"*). The reasoning behind
each answer is in `notes/ir-questions-explained.md`.

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

## Proposed, not decided

| Question | Proposed | Why |
|---|---|---|
| The first target | **x86-64** | it is the machine here; there is no `qemu-aarch64` or ARM `as` on it, so ARM64 needs them in the suite |
| How `hdc` reaches the tool | **two steps, as today**: `hdc` writes text, a `Makefile` runs the tool | record 0010: the compiler is told and never searches |
| The text's dialect | **Intel order, one instruction per line, a subset `as` reads** given `.intel_syntax noprefix` | the oracle needs both to read it; anything that is only ours is stripped for `as` |
| A branch on x86-64 | **always the four-byte form** | wastes space and gives exact sizes with no relaxation, which WCET wants |
| A conditional branch out of ARM64's ±1 MB | the inverted branch over a `b`, chosen by a fixed rule | same reason; to be settled at ARM64 |
| The calling convention | **our own, free of the C ABI** (answer 74), starting from the System V register order | system calls are close to it and gdb reads it easily; aggregates through a hidden pointer, as the IR's memory form has them |
| Register allocation | **none at first**, every value in its slot | IR memory form, record 0077; slow and simple, and correctness comes first |
| How sizes reach the compiler | **a map the tool writes** (each label's address and size) | WCET and placement need them; later, in the same process, a call |
| The debug format (76) | **DWARF**, written by our tools, built after the first back end runs | it is what gdb reads; a format of ours would need a debugger of ours. Positions per instruction are already kept (question 86) |
| A floating point `floor` and `pow` | `ffloor` an instruction; `pow` in Haard, in `std` | **no libm any more**, which forces what `docs/PENDING.md` section 5 item 1 kept open: the same result on every target, where WCET points, or libm's precision |

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

## The order

1. The assembler in Haard for x86-64, refusing what it does not know, with `as` as the
   oracle in its tests.
2. The natives and `_start`, with a small program running with no C.
3. The back end in `hdc`, running `tests/emitter` natively as a third path beside C++
   and the IR, which are its oracle.
4. The bootstrap's steps 2 and 3.
5. ARM64, then the processor, through the same tool.
6. Debug information, then PIE as the default before a public version.
