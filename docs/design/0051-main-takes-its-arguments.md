# 0051 — `main` takes its arguments, in two shapes

Status: **decided and built**, 2026-09-09. Hadley asked for both.

| | |
|---|---|
| `main` may take `(argc, argv)`, C's own pair | **decided**, Hadley |
| `main` may take **one list**, `@args : String[]` | **decided**, Hadley |
| The list form knows **no library class**: it asks for `add` **by name** | **decided while writing it** |
| A list that owns something and says nothing about copying is **refused** | **found while writing it** |

## What was wrong

`Emitter::emit_main` looked for a `main` and skipped every one that took
anything:

```cpp
if (candidate == 0 || query.get_params(function).size() > 0) {
    continue;
}
```

and then said *the entry module declares no 'main' that takes nothing*. A
program could not see its own command line at all — and a compiler written in
Haard is a program whose entire input is a command line. It is the second of
the three things
[notes/bootstrapping-haard-in-haard.md](notes/bootstrapping-haard-in-haard.md)
measured as blocking.

## The two shapes

```haard
def main : i32
    @argc : i32
    @argv : char**
```

C's own pair, handed straight through. Nothing is built and nothing is freed;
the shim is two lines, and `int` is cast to whichever whole number the program
wrote, because which one it writes is the program's business.

```haard
def main : i32
    @args : String[]
```

One list, built in the shim out of `argc` and `argv`:

```cpp
int main(int argc, char **argv) {
    h3_42_Array args;

    for (int i = 0; i < argc; i++) {
        args.m_add_n2c1(h2_1_String(argv[i]));
    }

    return h0_1_main(args);
}
```

**And the emitter knows neither `Array` nor `String` to write it.** It asks
the parameter's class for `add` **by name** — the way record 0040 asks a
container for `iterator` — and takes what that method's parameter is as the
element, building each one out of the `char*` by record 0045's construction. A
reader's own two classes work exactly as the library's do, which is what
`tests/emitter/cases/main_takes_its_arguments_as_a_list` is written with.

Naming `Array` here was the obvious alternative and was declined for the
reason records 0045 and 0046 declined it: a compiler that knows one library
class by name cannot be a compiler two libraries can share.

## What writing it found

**The shim is the one call in a program that no Haard source wrote**, so
nothing else was going to ask record 0031's question about it. The first draft
of the case had a list that owned a `new Word[8]` and declared no copy, the
shim handed it over **by value**, and what came out was a double free g++
wrote without a word. The emitter asks now, and says so by name.

## What is not decided

**Overloading `main`.** A program declares one, and the shim picks the shape
that fits it. Three `main`s in one file is not refused and not meaningful; the
first that fits wins.

**Anything about the arguments' encoding.** `argv[i]` is whatever the C
runtime handed over, byte for byte.
