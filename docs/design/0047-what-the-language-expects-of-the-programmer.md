# 0047 — What the language expects of the programmer

Status: **decided**, Hadley, 2026-09-09. This one decides no mechanism. It
writes down the **stance** the mechanisms are chosen from, because the
conversation that produced it settled four questions at once and the same
question will keep coming back in a new costume.

Read it before proposing that the compiler refuse something.

## The stance

> Haard's philosophy is that the programmer is intelligent and knows what they
> are doing. And that if something goes wrong, they will know how to fix it.
> The compiler does not need to babysit the programmer as though the programmer
> were stupid. They are not.

And, on `T&` carrying two jobs at once:

> In Haard it is deliberate that it is (a) and (b) at the same time, always.
> [...] The compiler must not get in the programmer's way, and the language
> expects the programmer to have the sense to know which case is which. The
> programmer is expected to read the documentation of whatever API they happen
> to be using, and to know what to expect of that argument.

## The rule that falls out of it

**The compiler refuses when it cannot decide, not when it disapproves.**

That line is not a slogan; it is what separates the four cases below, and it
was Hadley's own reading of why the ambiguity error is right and C++'s
reference error is not. An ambiguous call says *I do not know which one you
meant* — a fact about the compiler's knowledge, and the answer is for the
author to say which. A refused temporary says *you probably did not want
this* — a guess about the author's intent, and not the compiler's business.

## The four cases this was settled on

Each was measured and run, not argued. Each is a program Haard **compiles
today**, and in each the verdict is Hadley's.

### 1. A write through a temporary is lost

```haard
def bump : void
    @c : Counter&
    c.n = c.n + 1

let value = 5
bump(value)          # builds a Counter(5), bumps it, discards it
println(value)       # 5
```

C++ refuses this — it is Stroustrup's `increment(i)`, and the reason is that
`T&` there means *out parameter*, so a bound temporary is a write nobody can
read. Haard allows it. Hadley:

> I expect the programmer to be intelligent enough to know that this is the
> kind of code one does not write under normal conditions. What I would expect
> of the programmer: that they be intelligent enough to write, by hand, a
> `bump` overloaded to take a numeric value.

### 2. A returned reference to a temporary dangles

```haard
def keep : String&
    @s : String&
    return s

let held = keep(p)   # p is a char*; the String built for it dies here
println(held)        # prints garbage
```

> If they expect the reference `keep` gives back to stay alive, it is their
> job in the first place to hand `keep` a valid reference with a suitable
> lifetime. To me, printing garbage in that context is exactly the expected
> behaviour.

### 3. A raw handle becomes a `File`

`std/file.hd` declares `init(@where : i8*)` to store its handle, so every
`i8*` in the program now converts to a `File`.

> If the programmer got as far as thinking of handing a `handle : i8*` to
> `close_it`, it is because they have already made sure by other means that in
> that context the handle really does point at something to do with a file and
> not at a matrix of pixels. [...] The problem is in the programmer and not in
> the compiler.

### 4. Two classes constructible from `char*` make a call ambiguous

This one the compiler **does** refuse, and that is the point:

> What I expect of the compiler is precisely that it give an error message,
> and that it push the programmer into writing one of these explicitly: either
> `send(Path(where))` or `send(Message(where))`, or else writing a `send`
> overloaded to take a `char*`.

Cases 1–3 are the compiler declining to guess at intent. Case 4 is the
compiler admitting it does not know. Only the second is its job.

## What C++'s rationale actually was

Kept so nobody re-derives it, and so that the disagreement is with a real
position rather than a caricature.

C++ is not against temporaries: `const T&` binds one **and extends its
lifetime**. The rule is about contracts, and `const` is the marker that splits
one syntax into two of them — `const T&` is *borrow to read*, `T&` is *borrow
to write, and the caller will read the result*. A temporary in the second is a
provably lost write, so C++ makes it a type error.

Haard has no `const` (record 0029, deferred), so it has one spelling for both
contracts. It keeps the spelling and drops the guarantee. That is a **trade**,
not an oversight: without it, record 0037's own motivating example —
`foo("abc")` against `foo(@s : String&)` — could not work at all, and every
read-only reference parameter in `std/` would need a keyword Haard does not
have.

One option stays on the table and belongs to Haard alone: the compiler
compiles the whole program and sees every body, so it **could** infer whether
a `T&` parameter is ever assigned through, and refuse a temporary only for
those. C++ cannot do this — a header declares without defining. It is recorded
here as available, not as proposed.

## The question this leaves open, and it is Hadley's

Hadley did **not** accept that the conversion set is right. He accepted the
behaviour *given* a conversion, and separately raised that the conversions
themselves may be too many:

> This program should not have compiled in the first place, because building a
> `Counter` out of an integer should not have been allowed. [...] The way the
> compiler has of knowing when it is and when it is not, is to keep a table of
> types that make sense converted one into the other. A double coming from an
> integer inside a numeric context makes every sense, philosophically speaking.
> A String coming from a `char*`, philosophically speaking, makes every sense
> too, since both are textual information the programmer already has a formed
> idea about. Now, does a `Counter` have the same semantic character as a
> classic numeric value? I believe not.

This is a different question from C++'s `explicit`, and a more interesting
one. `explicit` is a per-constructor opt-out written by the class's author.
What Hadley describes is **kinship between types** — textual things convert to
textual things, numeric to numeric — which is a property of the pair and not
of one constructor.

Two measured facts belong with it, because they change the shape of the
question:

- **Haard already refuses `i32` → `f64`**, with no class anywhere: `let a :
  f64 = n` is *expected f64, found i32*. Record 0018 has no numeric conversion
  at all. So the example given as obviously sensible is one the language does
  not do today — and `Counter(5)` converts only because its `init` takes an
  `i32` **exactly**. An `init(@n : i64)` would not have compiled.
- **Haard requires the constructor's parameter type exactly**, where C++ runs
  one standard conversion first. Neither chains two constructors.

| the `init` takes | given | C++ | Haard |
|---|---|---|---|
| `i32` | `i32` | converts | converts |
| `f64` | `i32` | converts | **refuses** |
| `i64` | `i32` | converts | **refuses** |
| `Thing&` | `Thing` by value | refuses | refuses |
| `Thing` | `Thing` by value | converts | converts |
| `char*` | `char*` | converts | converts |
| `i8*` | `char*` | refuses | refuses |

So the conversion set is already narrow, and narrow in an accidental way: it
is narrow because record 0018 has no numeric conversions, not because anyone
decided which types are kin. Agenda **1.37** is that decision, and it is open.

## How this governs the next decision

When a choice comes up between the compiler allowing something and refusing
it, this record says which way Haard leans, and it is not "always allow":

1. **A refusal must be about the compiler's knowledge, not the author's
   intent.** *I cannot tell which* is a diagnostic. *You probably meant
   something else* is not.
2. **A missing guarantee is not a bug to be fixed by a refusal.** Lost writes,
   dangling references and silent conversions are the author's to avoid.
   Record them, do not outlaw them.
3. **When a mechanism would need a keyword the language does not have**
   (`const`, `explicit`), that is a reason to leave the mechanism open, not a
   reason to refuse the program in the meantime.
4. **A diagnostic that names the two ways to write it is worth more than a
   permission**: the ambiguity error in case 4 is the model — it says what it
   does not know and leaves two spellings that both work.
5. **Whether two types may convert is a question about the pair**, and it is
   open (1.37). Do not answer it one constructor at a time.

And what this record does **not** license: it is not an argument against
checks in general. Every check the compiler already makes — the override
checker, `may_be_copied`, an exhaustive `switch`, a call that matches nothing
— answers *I cannot tell* or *this cannot be built*, never *you should not
want this*.
