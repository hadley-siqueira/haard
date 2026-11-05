# hasm

The assembler and linker of record 0084, written in Haard: text in, a static ELF out,
with no `as`, `ld` or libc. x86-64 Linux only so far.

```
make                       build build/hasm
make test                  build it and run tests/run.sh
hasm <input.s> -o <output> and then 'chmod +x', which Haard cannot do yet
```

| file | what it is |
|---|---|
| `hasm.hd` | `main` |
| `assembler.hd` | a file into an image: labels, directives, diagnostics |
| `word.hd` | one line cut into words |
| `x86_64.hd` | one instruction into its bytes |
| `image.hd` | the sections, the labels, the holes, and where everything goes |
| `elf.hd` | an image written as a file |

There is one pass over the text and no relaxation: a branch is always the long form,
so every size is known as it is written, and the holes are filled once at the end.

## What it reads

One statement per line, in a subset of what `as` reads under `.intel_syntax noprefix`.
**Whatever is not listed here is refused, by line**, all of them at once, and then no
file is written.

**Labels** stand alone on a line, quoted when they have to be:
`"std.io.print(i32)":`. One starting `.L` is not in the symbol table. `_start` has to
exist, and is where the program begins. A label's size in the symbol table runs up to
the next label of its section that is in the table.

**Directives:** `.text`, `.data`, `.bss`, `.section .rodata`; `.byte`, `.word`, `.long`
and `.quad` with comma lists; `.quad label`; `.ascii` and `.asciz` with the escapes
`\n`, `\t`, `\\` and `\"`; `.zero`; `.balign`, except in `.text`.

**Operands:** the sixteen general registers at each size (not `ah`, `bh`, `ch`, `dh`),
`xmm0` to `xmm15`, numbers in decimal or `0x`, and memory as
`[base + index*scale + disp]` or `[rip + label]`, with `byte`, `word`, `dword` or
`qword ptr` before it.

**Instructions:**

| |
|---|
| `add or adc sbb and sub xor cmp mov test lea movzx movsx movsxd` |
| `not neg mul div idiv imul rol ror shl shr sar` |
| `push pop jmp call ret leave nop cdq cqo syscall ud2 mfence` |
| `j*`, `set*`, `cmov*` with `o no b ae e ne be a s ns p np l ge le g` |
| `add sub mul div min max sqrt`, each with `ss` and `sd` |
| `ucomiss ucomisd comiss comisd cvtss2sd cvtsd2ss movss movsd movd movq` |
| `andps andpd orps orpd xorps xorpd` (between registers) |
| `cvtsi2ss cvtsi2sd cvttss2si cvttsd2si roundss roundsd` |

## What it writes

A page-aligned segment for each section with something in it: `.text` read and run,
`.rodata` read, `.data` and `.bss` read and write, loading from `0x401000`, and a stack
that cannot run. Every label in the symbol table is `STB_LOCAL`, since a program is
compiled whole.

## The tests

`tests/run.sh` needs `as`, `ld`, `objcopy`, `objdump`, `nm` and `readelf`, as an oracle
and nothing else.

| | |
|---|---|
| `tests/forms.sh` | every form it knows, generated: the same bytes as `as` |
| `tests/combinations.sh` | every instruction with every shape of operand: what it does not refuse has the same bytes as `as` |
| `tests/cases/same` | by hand: the bytes, addresses, sections, symbols and entry against `as` and `ld` |
| `tests/cases/refused` | it says no, in these words, and writes nothing |
| `tests/cases/runs` | it runs with no C, with this output and exit code |
| `tests/cases/symbols` | what `readelf` reads of names, addresses and sizes |
| `tests/cases/headers` | what `readelf` reads of the segments and their permissions |

Comparing with `as` takes two adjustments, both in `run.sh`: the file gets
`.intel_syntax noprefix` and `.globl _start`, and each branch to a label gets `{disp32}`.
`tests/layout.ld` puts things where `hasm` does.
