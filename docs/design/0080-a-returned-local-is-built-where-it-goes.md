# 0080 — A returned local is built where it goes

Status: **decided**, 2026-11-03, Hadley chose option (a) of three: *"3a"*.
It was **already built** on both paths, since IR stage 4 step 3
(2026-10-28), and had never been written down. The other two options were a
guaranteed elision or move for every returned local (which needs an implicit
move, and record [0066](0066-a-move-is-written.md) says a move is only ever
written, `&&x`), and always copying.

| | |
|---|---|
| When **every** `return` of a function gives back **the same local**, that local is built in the caller's destination and never copied | **decided**, Hadley |
| Otherwise a returned local is copied into the destination, by the class's copy `init` when it has one | **decided**, Hadley |
| It is observable, through a copy `init` that does something, and is the language's rule and not an optimisation | **decided**, Hadley |

## Why

It is g++'s rule (the *named return value optimisation*, which C++17 allows
and g++ applies at every optimisation level), so the C++ path already
followed it, and the IR was built to agree with the C++ path. Writing it down
makes a program that counts its copies give the same answer on every back
end, the processor's included, rather than whatever the back end happened to
do.

```haard
def make : Big
    let b = Big()
    b.fill()
    return b            # every return gives back 'b': built in 'x'

let x = make()          # no copy 'init' runs

def pick : Big
    @first : bool

    let a = Big()
    let b = Big()

    if first:
        return a        # two locals: the one given back is copied
    return b
```

The rule is syntactic on purpose: it can be checked by reading the function,
and the price is that changing one `return` can add a copy.

## How it is built

The IR's `FunctionLowerer` finds the local every `return` names before the
body is lowered and gives it the destination pointer instead of a slot of
its own (record [0077](0077-an-ir-for-the-processor.md), step 3). The C++
path writes the local and the `return` as they are, and g++ does the rest.

Not covered: a C++ compiler other than g++ is not obliged to elide, so the
rule holds on the C++ path because of which compiler it uses. A back end of
our own (the processor, x86-64, ARM) follows the IR.
