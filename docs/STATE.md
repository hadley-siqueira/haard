# Where the compiler is

Written 2026-09-02, last brought up to date 2026-09-09. The agenda of
*decisions* is `design/README.md`; this is the state of the *code*, and what to
do next.

## What `hdc` does today

`hdc file.hd` — one file, imports not followed, when no `haard.pkg` is at or
above it.
`hdc entry.hd` — the whole program, when one is: the nearest manifest is found
and read (record 0044).
`hdc --pkg app/haard.pkg entry.hd` — the same, said out loud.
`hdc --roots table.tbl entry.hd` — the program and everything it reaches.
`--pretty-print` writes the source back from the tree; `--tokens` dumps the
token stream; `--emit-cpp` writes the program as C++.

**`--pretty-print` stops after the parser**, since 2026-09-03. It used to run
every phase and report names and types nobody had asked about, so a file that
printed back perfectly still exited 1. The printer's subject is a **file** and
those questions are about a **program**. `Compilation::stop_after_parsing` is
the switch, and it also means the imports are not followed.

**It emits C++, since 2026-09-03.** `hdc --emit-cpp` writes the whole program
as one C++ file for a C++ compiler to turn into a binary — record 0025, track
5. No intermediate representation: record 0019's ResolutionTable already
carries a type and a declaration per node, and C++ is not lower than the
source. Hadley has said an IR **will** be needed for a processor he is
building; 0025 names the signals that will say when.

What it refuses, it says so about, rather than writing C++ that means something
else: a hash literal, a tuple, a list, a closure, a range, and `T[]` with no
length **inside a type**. Two have left that list: a **template string** on
2026-09-06, taken apart before the emitter is reached, and a **symbol** on
2026-09-08, which is now a builtin and an entry in a table the emitter builds
(record 0041). `new T[n]` is not that case and works — record 0028.

**A whole program, the way a user builds one**, is
`tests/programs/cases/<name>/Makefile`: `hdc --roots table entry.hd --emit-cpp`
into one C++ file, then a C++ compiler. There is no build system inside `hdc`
— it is told where the roots are and searches nowhere else (record 0010), so
the roots table is the whole of what a build system has to generate. The suite
builds through those Makefiles, which is what keeps them true.

`Compilation::build` is six walks. Each split was forced by the same rule:
*a phase that reads across modules cannot run inside the walk that fills them.*

| # | walk | what it does |
|---|---|---|
| 1 | load + resolve imports + collect symbols | per module, in the loop that discovers them |
| 2 | collect types | what each declaration **wrote** |
| 3 | infer types | what each binding was **given**, and where record 0040's `for x in` is taken apart |
| 4 | resolve uses | every name reported that nothing declares |
| 5 | check statements | `return`, conditions, assignments |
| 6 | check overrides | the return type of an override |

Seventeen test suites, ~680 cases, `make check` in about a minute.

## What works, proven by running it

Classes, single inheritance, methods, fields, `init` and `destroy`, `.` and `->`, `this`, overloads
chosen by signature, overriding, inherited names by a bare name, imports with
aliases and star imports, two versions of one library in one program, `let`
optional, statement checking, rustc-shaped diagnostics, record 0018's coercion
list wherever a type is given to something — a call, a return, a binding, an
assignment — including `char*` to `String`.

A non-trivial program compiles clean and its hand-written C++ translation runs
and gives the right answer, which is what says a transpiler is the right shape
of backend.

**A construction can be written**, since 2026-09-09 — record 0045.
`String("abc")`, `Point()`, `Pair<i32, i32>(1, 2)` and `i32(x)` all build what
they name, and a construction is **a call whose callee names a type**: no node
kind was added and no pass runs, `ExpressionTyper::call` asks what the name
means before anything is ranked and branches to record 0026's own
`initialisation`. The value is a **temporary written in place**, so it reaches
a call, a return and an assignment — which is the half records 0032 and 0037
cannot reach, because they hoist.

It **deleted** record 0023's two entries in the same work, and record 0046
replaced them the same day with one that names **nothing**: a value reaching a
class it can be constructed from converts, by the class's own `init` and a
temporary. So `takes(p)` works again, `String(p)` is the same thing written
out loud — byte for byte the same C++ — and record 0018's list still names no
library class.

**There is a standard library and there are examples, at the repository root**,
since 2026-09-08. `std/` is the canonical library — seven modules, all Haard —
and `examples/` holds two runnable programs: `hello`, which is `import std.io`
and one `println`, and `shapes`, an application and two libraries that reach
each other through their manifests. Both are in the README, and both were run
before it was written.

**A program can be built from its manifests**, since 2026-09-08 — record 0044.
`haard.pkg` is a **Haard literal** (`{name: "myapp", dependencies: [{name:
"std", path: "../std"}], prelude: [...]}`), read by `Parser::parse_value` — an
entry point of its own — so there is no parser to write and a typo in a
manifest is a diagnostic with a caret. `hdc --pkg` follows the path
dependencies, reads each library's own manifest and builds record 0010's table
itself; nothing below the finder can tell which of the two was read. Versions,
a registry and a lock file stay open, and a dependency asked for by version is
refused by name.

**An enum is a tagged union**, since 2026-09-08 — record 0043, and Hadley's
semantics and syntax whole. A variant that carries something is a
**constructor** (`Action.Click(10, 20)`, or bare `Click(10, 20)`, which a name
in scope wins over); the payload-free enum is the degenerate case and compiles
to a C++ `enum class`; anything else is a struct with a tag and a union, with a
maker per variant and — when a payload is a class — a destructor and a copy
that ask the tag which member is alive.

Two of them compare with `==`, which the emitter writes as a `__equals` on the
struct: the tag first, then the payload, and a class payload by the
`operator==` it wrote. A class that wrote none simply leaves the enum without
`__equals` — the enum is still built, matched and destroyed like any other, and
only **comparing** two of them is refused, at the comparison and by name
(`tests/emitter/cases/an_enum_that_cannot_be_compared`).

It is read by **`switch`**, which is a pattern match: **no fall through and no
`break`**, cases group by writing one with no block, a `case` captures by
**reference** into the value being switched over, and a switch over an enum is
**exhaustive or writes `default`**. Over an integer or a char it is a C++
switch; over a String, a float or any class with `==` it becomes a chain of
`if`s.

**The standard library's four classes are all written**, since 2026-09-08:
`Array<T>`, `List<T>`, `String` and now `Hash<K, V>` — record 0042. The Hash is
**open addressed** (three parallel buffers, linear probing, tombstones, growth
at half full) and hashes its key by an **overload set**, `hash_of`, since there
are no interfaces to ask a type with: the clone resolves it against the K it
was instantiated with, and a key type nobody wrote one for is reported at the
instantiation. `i32`, `char*`, `String` and **symbol** keys are written.
Walking one gives back its **keys**. It is in the prelude beside the others.

**A symbol is an interned name**, since 2026-09-08 — record 0041, and Hadley's
design whole. The emitter builds one immutable table for the program, one entry
per distinct name, and every `:foo` anywhere is the pointer to that entry — so
comparing two symbols is comparing two addresses, and the entry gives the text
back through `s as char*`. It is a **type of its own**, `symbol`, and a
keyword, so a parameter and a field may be one; `:foo == "foo"` is a mistake
about types, which is the Ruby distinction it was asked for.

**A written literal reaching a class is a construction the compiler chose**,
since 2026-09-08 — record 0037's mechanism, now carrying `char*` → `String`
too. The typer picks the `init` and writes it on the literal; the emitter
writes that call at a binding, a call, a return and an assignment. The name
`String` no longer matters for a literal — any class with an `init(char*)` is
reached — and since 2026-09-09 it does not matter for a **value** either:
records 0045 and 0046 replaced the named entry with one that asks the class
for a constructor, so a `char*` value reaches any class with an `init(char*)`
and reaches no other.

It found a bug shipped since record 0031: `s = "abc"` on a class with no
`operator=` emitted `s.m_assign("abc")`, which **g++ refused** — the tenth
silent bug, and the first found by reading the emitted C++ rather than by
running a program.

**`for x in` walks a container, a fixed array or a range**, since 2026-09-08 —
record 0040. A class says how it is walked by answering `iterator()`,
`has_next()` and `next()`; a fixed array is walked by the length its type
carries; a range is a plain loop with no class. The loop variable is a
**reference** to the element, `break` and `continue` mean what they say, and
`tests/programs/cases/containers_are_walked_by_for_in` walks an `Array<i32>`, a
`List<i32>`, a fixed array and both spellings of a range and checks its own
work.

**Everything about construction is `init`**, since 2026-09-06 — record 0038,
amending record 0031. There is no method named `copy`: the copy constructor is
the `init` whose one parameter is the class itself. It forecloses `from`,
`convert` and every other name, and leaves the room a simplified move will
need — `init` overloaded on `&&`, once `T&&` parses. Everything record 0031
decided still holds; only what says it changed, and the emitted C++ is byte for
byte the same but for the method's name.

**A `char*` reaching a `String` is a call the compiler writes**, since the same
day. It used to compile and name no constructor: record 0026 emits `init` as a
C++ constructor of one argument and **C++'s own converting constructor** ran
it, which is record 0034's rule surviving in the compiler's own emission.

```cpp
h0_1_String h0_17_built("abcd");            // let built : String = "abcd"
by_value(h0_1_String("abcdef"));            // by_value("abcdef")
h0_17_built.m_operator_eq_pb12("abcdefg");  // built = "abcdefg"
```

The pair stays **known by name** — Hadley chose that over a general converting
constructor: a class of your own writes the overloads by hand and converts in
them. `char*` → `String&` is one more entry on record 0018's list and **not**
the list learning to compose, which is still open.

**`=` is overloadable** and **record 0031's assignment is a method**. `a = b`
between two class values is `a.m_assign(b)` now, and no `operator=` appears in
the emitted C++ at all. Not `m_copy`: an assignment **destroys first**, and
calling the copy alone would leak what the target held.

**A clone is typed when it is made**, since 2026-09-06 — record 0039. Before
it, `let xs = [1, 2, 3]` followed by `xs.length()` was *no 'length' takes these
arguments*: inferring the literal clones `Array` into the module that declared
it, and the module asking had already been walked. **Writing the type anywhere
in the program hid it**, which is how it survived every case — each had a
helper taking an `i32[]&`.

**An empty `[]` takes its type from the context**, since 2026-09-06. `let a :
f64[]` then `a = []` works and `let b = []` is an error that says to write the
type. A **call** is not context — an argument is typed before the overload is
chosen — and that is deliberate: infer where it is trivial, report where it is
not. It is the one literal the sugar pass leaves where it stands, because what
a literal is hoisted for is the fixed array it is made of and an empty one is
made of nothing. It emits as `Array<T>(nullptr, 0)`.

**A bracket literal builds an `Array<T>` and `List<T>` exists**, since
2026-09-06 — record 0037.

```haard
let xs = [1, 2, 3]                  # Array<i32>
let a : List<i32> = [1, 2, 3]       # List's init(Array<T>&), chosen and called
let b : List<i32> = {1, 2, 3}       # List's init(T*, i32)
```

Everything reduces to `{...}` plus one constructor call. The fixed array is a
**file-scope `static`** when every element is a literal and a local when not,
and one `add` per element became one call. **Only a literal** is translated
into a constructor call — `let l : List<i32> = some_array` is refused, because
the alternative is C++'s converting constructor firing silently for every
class.

`List<T>` is a doubly linked list with a head and a tail, written in Haard,
with `operator[]` that walks. It needed two holes closed: `T[N]` did not decay
to `T*`, and the body of an **unbound generic** was being checked — two
occurrences of `Node<T>` in two generics are two types, so `walk = walk->next`
was *cannot assign Node<T>* to Node<T>**. Record 0002 already said the clone is
what a use names, so the statement checker skips a generic nothing has bound.

**`Array<T>` works and `T[]` is written form for it**, since 2026-09-06 —
records 0016, 0022 and 0036, and it needed generics *and* the prelude before it
could be built at all.

```haard
let xs : i32[]          # Array<i32>
xs.add(3)
xs[1] = 100             # written through the T& that operator[] gives back
let grid : i32[][]      # Array<Array<i32>>
let ys = xs             # its own buffer, record 0031
let same = xs == ys     # record 0034's operator==
```

The sugar is a rewrite in record 0025's Ast → Ast pass, beside the template
strings. `T[3]` is untouched — record 0021 makes a written length a **fixed**
array, which is not a class.

**Operators are methods with names no source can write**, since 2026-09-06 —
record 0034. `def operator[] : T&`, written against the word, and the name is
`operator[]`, which no identifier can be. So nothing after the parser knows an
operator was involved, and **the C++ does not use C++'s operator overloading**:
`xs[3]` comes out as `xs.m_operator_at_b6(3)`. Hadley's rule, and record 0025's
argument against templates a second time.

**A reference is the thing it names**, since 2026-09-06 — record 0035, C++'s
semantics, and record 0018's list amended a second time. Reading one costs a
step and asks record 0031's question first, because it is a copy. Assigning to
one writes **through** it, which changed a recorded behaviour: `b = d` between
a `Base&` and a `Derived&` used to pass and is slicing.

**Template strings work**, since 2026-09-06 — record 0032, and the front end
had been ready since the 5th. `"n is ${n} and twice that is ${n * 2}"` is a
local `String` and four `append` calls, written before the statement it was
written in by `src/haard/sugar/sugar_lowerer.{h,cpp}` — record 0025's
Ast → Ast pass, which nothing had needed until now.

Nested works: `"outer ${"inner ${n}"} end"` builds the inner one first, because
the pass walks a template string's children before lowering it and every
insertion goes directly before the same statement. Each `${}` finds the
`append` its own type asks for, so String's eight overloads carry the whole
feature and no phase after the pass knows a template string existed.

**Three places are refused**, and loosening them is additive: an operand of
`and`/`or` (evaluated only when the left did not decide), a loop condition
(built once instead of every turn), and module level (no statement to be built
before). A refused one is still lowered into a `String` with the appends
dropped — recovery, so one mistake reads as one error.

**The prelude works**, since 2026-09-06 — record 0033, and it is not record
0017's prelude. There is **no prelude module**: the roots table carries a
`prelude` block listing imports that every module of the program is given,
appended to its dependency list **after** the ones the source wrote. A file
that imports nothing uses `String`, `File` and `console()` and compiles, and
`tests/programs/cases/the_prelude_is_written_in_the_table` is that program,
built by the real `hdc` through its own Makefile.

Record 0009's *the prelude after the imports* took no code: `get_dependencies()`
has two readers and the flat walk already reads the list in order. Record 0008
keeps **no exception at all** — every name still arrives through an import, and
the table writes some of them.

## What does not work, and it is mostly silence

**The emitter carried none of the language's precedence** until 2026-09-10.
It wrote every binary operator as `left oper right` with nothing around it, so
**C++ regrouped every expression the two languages read differently** — which
is five levels of the table, since `**`, `|`, `^`, `&` and the shifts bind
tighter than `*` here and looser than `+` there. `println(4 + 3 & 1)` printed
1 where the tree says 5, `println(1 << 2 + 3)` printed 32 where it says 7.
`hdc` was right, g++ was right and the program was wrong, with no diagnostic
anywhere. Not one of the 37 emitter goldens moved when it was fixed: **no test
had ever written an expression the two languages disagree about**, which is
exactly how a defect of this shape lives for as long as it did. Record
[0057](design/0057-the-precedence-of-the-operators.md), which also moved `not`
above the comparisons and made `**` right associative.

**A call whose answer is thrown away** was typed by nothing until 2026-09-02 —
`takes_int(2.5)` passed in silence — and now is. **The increment of a C shaped
`for`** was the same hole and lasted a day longer: `for i = 0; i < 3;
takes_int(2.5):` passed until 2026-09-03, found by the emitter refusing to
name something the type phase had never looked at.

**Record 0018's coercion list reached only a call** until 2026-09-03, so
`return d` from a function giving back a `Base&` was refused. Fixed, and the
list lives in one place now.

**A field inherited across a module boundary was invisible to a bare name**
until 2026-09-03 — record 0020 worked inside one file and nowhere else,
because the lookup skipped its whole scope walk when the importing module had
never interned the name. Right for its own scopes, wrong for the base chain.
Found by writing a program of four modules and running it.

**`new T(...)` and `delete` were checked by nothing** until the same day.
`new Counter(2.5)` against an `init` taking an `i32`, and `delete 5`, both
passed in silence.

**An inherited field could be read by a bare name but not written** until
2026-09-05 — record 0027. `n = 7` in a method of a derived class declared a
**local** in front of the field, because deciding that a bare name declares was
done where symbols are collected and a base is a type, filled by the pass
after. So the field was never written, the local never read, and nothing said.
The decision moved to the type phase; a derived class redeclaring a base's
field is an error now.

**A bracketed expression could not be spread over lines** until 2026-09-03 —
the line rule reached inside a `{`, so a hash written over four lines did not
parse. Record 0024, and the rule is now suspended while a bracket is open.

**One expression kind still types to nothing**: `0..10`, which is waiting on a
decision — nothing has said whether a range is a type — and which `for i in
0..10` does not need, since record 0040 reads it as syntax. `"text"` types as a
`char*` (record 0022) or as the class it is being given to (record 0037), and
`:symbol` has been a type of its own since 2026-09-08 (record 0041).

A string literal is a `char*` and types today; the other eleven were fixed on
2026-09-02 — `a[i]`, `&x`, `*p`, `-x`, `~x`,
`++x`, `x++`, `x as T`, `new T`, `sizeof`, a tuple, a list and an array
literal, and `null`. Agenda **2.10**, closed.

**Generics work**, since 2026-09-05 — record 0002, made real. A generic written
with arguments is **cloned** into a declaration of its own, in the module that
**declared** the generic, under a name no source can write (`Pair#3#7`, the
arguments by their type index). Its type parameters are bound by giving their
candidates a type, so inside the clone `A` simply *is* `i32` and no walk
substitutes anything. Everything after that point reads an ordinary class:
overload resolution, the statement checker, the override checker and the
emitter did not have to change.

**C++ templates were the other way out and are ruled out**, because they break
record 0020. A method is named by its own name and its parameters, which is how
a derived class overrides a base's — and inside a `template`, `def push(x: T)`
mangles `_T` while `class IntStack : Stack<i32>` writing `def push(x: i32)`
mangles `_b6`. Different names, and C++ declares a second virtual function
instead of an override. Monomorphisation makes both `_b6`.

**A `Pair<i32>` where two arguments were written is reported**, and so is an
instantiation nested more than sixteen deep — `Pair<Pair<Pair<...>>>` is a
program that never stops instantiating and nothing about it is a mistake the
source can see.

**An override was not seen from outside the module that declared it** until
2026-09-05, and the cause was one line. `OverloadResolver::holder_of` gave back
the type index the holder's **own** module had, and `Coercion::climb` reads the
**caller's** — a class of module 1 looked up among the types of module 0, which
is record 0016's rule broken: a type index does not cross a module boundary.

What it cost was not one program. The index landed on whatever the caller's
table happened to hold at that number, so the answer came from an unrelated
table. Three outcomes, and the source being read decided none of them: a false
*"matches more than one 'describe' equally well"*; an accidental pass; or —
when the calling module declared a derived class **above** its base, which
Haard allows — the climb ran backwards, the **base** was taken to override the
derived, and nothing was reported. That last one silently narrowed a binding:
`let v = c.clone()` on a `Car` gave `v` the type `Vehicle*`.

The wrong *method* was harmless, and only by coincidence: a method is mangled
by its name and its parameters, and `overrides` forces those equal, so both
candidates carry one C++ name and the vtable dispatched correctly. The wrong
**return type** had nothing to save it.

Every suite was green through all of it. No case called a method on a value of
the **derived** type from outside the declaring module — every multi-module
case reached an override through a **base** pointer, where the lookup finds one
candidate and the resolver is never asked. Two cases now do:
`tests/emitter/cases/an_override_is_seen_from_another_module`, whose verdict is
the exit status, and the type_table case of the same name, whose golden shows
the covariant return as `Car*`.

**The brace literal types to nothing.** There is no `AST_HASH` case in the
typer at all. What it means is agenda 1.23, deferred on 2026-09-03 with the
design written down in `design/notes/what-a-brace-literal-is.md`: it is not a
hash, it is a composite literal read by its context, and the dynamic JSON
object is a library type and not a language one. Its **key** already does not
go through the scope chain, which every reading of it agrees on.

**There is no prelude**, so a string literal and a symbol literal have no type
and `T[]` is not `Array<T>`. Record 0017 decided the mechanism and left four
things open. Record 0021 adds a fifth: `[1,2,3]` is a **dynamic array** and
`{1,2,3}` a **fixed** one — the typer builds both correctly today — but the
dynamic one is a `TYPE_LIST` and should be an `Array<i32>`, and nothing has
said whether `[T]`, `T[]` and `Array<T>` are three spellings of one thing.

**`init` and `destroy` work**, since 2026-09-03 — record 0026. They stay
ordinary methods and are emitted twice, once as themselves and once as a C++
constructor or destructor calling them. A field with no value written is not
initialised; the destructor is virtual always; a base whose every `init` needs
an argument makes the derived unbuildable and is reported, since there is no
syntax for a base's arguments yet.

**`new T[n]` is a `T*`**, since 2026-09-05 — record 0028, and C++'s answer.
`T[]` is already sugar for `Array<T>` and `Array<T>` is what you build with
this, so it could not also be what this gives back. The length is not in the
type, so `delete` and `delete[]` are the author's to tell apart — which record
0023 had already assumed by giving `String` its own `capacity`.

The length is an **expression read off the tree**, so a non-literal one works:
`new char[capacity]` compiles, while `let buf : char[capacity]` still needs
agenda 5.3's constant evaluation. Nothing typed that expression until this
record — it lives under a **type** node and a type resolves no names, so
`new char[n]` died in the emitter with *"'n' names no declaration"*.

**`delete[]` needed nothing.** It was already whole through every phase, and
the typer already refused `delete[] 5`. What it deletes could simply never be
created.

**`const` is a convention and nothing more** — record 0029, deferred
2026-09-05. `const greeting = "hi"` emits `char *greeting = "hi"`, byte for
byte what `let` emits, and there is no `const` in the type system at all: no
qualifier among the eleven `TypeKind`s, and `let p : const char*` does not
parse. So **writing through a string literal compiles and is undefined** —
g++ warns on every one of them and the program segfaults. The record writes
down the semantics Hadley wants and the design that delivers them, so the
thinking is not done twice.

**Haard talks to the world**, since 2026-09-05 — record 0030, and temporary on
purpose. The emitter writes the body of **eight** functions, by name and only
inside the module named `std.low_io` (`std.io` until 2026-09-08). Nothing was added to the language: a `def`
whose body is `pass` already emitted a whole function with an empty body, and
the emitter fills it in.

C's `stdio` and not C++'s streams, because a `FILE*` is an opaque pointer and
is an `i8*` here with nothing lost, while an `std::ofstream` is an object Haard
cannot name. And `stdout` is a `FILE*` too, so **one set of primitives covers
the console and a file alike**.

One character at a time, and everything above it is written in Haard:
`std/io.hd` has `print` and `println`, overloaded on `char*`, `String&`,
`char`, `i32`, `i64`, `u32`, `f64`, `bool` and `symbol`, and `std/file.hd` has
`File`, `console()` and `open_read`.
`tests/emitter/cases/input_and_output` prints a line to the terminal, writes a
file, reads it back character by character and returns the count, and
`tests/programs/cases/printing_is_a_free_function` prints one line per
overload.

**`print` and `println` are free functions in `std.io`**, since 2026-09-08, and
that is the whole of what a program needs to print:

```haard
import std.io

def main : i32
    println("hello, world!")

    return 0
```

They stand on the natives directly and not on `File`, because `console()`
builds one with `new` and a `print` written over it would leak one per call.

**A char literal was a float literal to the typer** until 2026-09-05.
`ExpressionTyper::literal` handles three kinds and split them into two, so
`let e : f64 = 'w'` passed in silence and the emitter wrote `double e = 'w';`,
while `let d : i32 = 'z'` was refused in words calling it *a floating point
literal*. Three families now, and a char literal is a `char` and nothing else.

**A member's type did not cross a module boundary** until 2026-09-05, in two
places, and both were record 0016's rule broken the same way `holder_of` broke
it: a type index read from the **declaring** module's table and used in the
**reader's**.

`ExpressionTyper::member` gave back a field's type untranslated, so a member
whose type came from a third module — `one : Point` on a class of `lib.holder`,
read from an app that imports only `lib.holder` — got whatever the app's table
held at that number. The diagnostic told on it by naming a class of the
**reader**: *"Holder has no member named 'x'"* about a `Point`, and *"F2 has no
member named 'x'"* once the reader declared two structs.

`ExpressionTyper::identifier` had it too, for a bare name reaching a global of
an imported module or a field of a base in one (record 0020). That one hid
behind the decision that makes an index portable: a **builtin holds the same
index in every module**, so every cross-module bare name anyone had written —
an inherited `n : i32` — was right by construction, and one of a class type
would not have been.

The audit after `holder_of` missed both because it was **line-based** and these
read across four lines. A multi-line search found them and says there are no
more.

**The six bitwise operators typed to nothing** until 2026-09-05. `a & b`,
`a | b`, `a ^ b` and the three shifts passed the check phase in silence, and
only the emitter noticed, by refusing to name something with no type — so
`hdc file.hd` called such a program fine and `--emit-cpp` did not. Agenda 2.10
counted fourteen expression kinds on 2026-09-02 and these were not among them.
They type like arithmetic now, with one rule more: a float has no bits to
speak of and is refused by name.

**Copying a value was undecided, and so a class that owned memory was freed
twice** — silently, and by all four of the ways a value is given to something.
Record 0031, 2026-09-05: `destroy` is what says a class owns something, and
one that owns and declares no `copy` may not be copied, reported at the call,
the return, the binding and the assignment. `copy` is the third of record
0026's family, emitted as a C++ copy constructor and copy assignment — the
assignment destroying first, since `copy` is written by a class holding
nothing.

Found from the other end: a template string is a `String`, a `String` owns its
bytes, and every shape that sugar could take passes one by value. **The
blocker was never template strings.**

**`String` exists and is its own builder**, since 2026-09-05, written in Haard
in `tests/programs/cases/strings_are_built_and_joined/std/string.hd`. It owns
its bytes (record 0023) and writes the `copy` record 0031 requires, so all four
ways of giving a value to something work and no two Strings hold one buffer.
`append` is overloaded for `char`, `char*`, `String&`, `i64`, `i32`, `u32`,
`bool` and `f64` — **so there is no StringBuilder and there does not need to
be**: what one would hold is what a String holds.

**Record 0023's coercion runs for the first time.** `let hello : String =
"hello"` works, and it falls out of record 0026 emitting `init` as a C++
constructor of one argument — which is what an implicit conversion is over
there. The entry it ran through is gone since 2026-09-09 (records 0045 and
0046) and that line still works twice over: a written **literal** is record
0037's construction and never was the entry, and a value now converts by the
constructor too.

**Record 0018's list gained a fourth entry**: a value where a **reference** to
it was expected. It had none, so a `&` parameter could not be passed a value —
which made record 0031's own advice, *hold it by reference*, impossible to
follow. Amended in 0031.

**A literal only learned its type from the left** until 2026-09-05. `big - 0`
worked and `0 - big` was *cannot apply this to i32 and i64*, because `binary`
typed the left first and handed its answer to the right. Whichever side is a
written number now waits for the other.

**Template strings work**, since 2026-09-06 — record 0032, and this paragraph
used to say the opposite. Skip to *What works* for what they do; what follows
is what they were.

**Template strings parsed and pretty-printed exactly, and typed to nothing**
until 2026-09-06. The front end is complete: the scanner emits
`BEGIN CHUNK (INTERP CHUNK)* END` and the tree holds the real expressions. What
is missing is everything after it. `AST_TEMPLATE_STRING` lives in the scanner,
the parser, the builder and the printer and **nowhere else** — not in the
ExpressionTyper, not in the StatementChecker, not in the emitter — so
`let s = "value is ${n}!"` passes `hdc file.hd` in silence and dies in
`--emit-cpp` with *'s' has no type the emitter can write*. The tenth instance
of the one recurring shape.

**Record 0032, 2026-09-06, decided how it lowers** and the answer includes a
*wait*. A template string becomes a **local `String`** and a run of `append`
calls, **hoisted** before the statement it was written in — not a chain giving
back a `String&`, which would change the library to suit the sugar and needs an
unnamed stack temporary, an Ast shape that does not exist. Hoisting out of an
operand of `and`/`or` or out of a loop condition changes the meaning, so it is
**refused** there and loosened later, additively.

**How the pass names `String` is the prelude's question, and Hadley answered it
with the prelude** — not with a required `import std.string`. So **the prelude
moves ahead of template strings** in the order of work, and record 0017's open
point 4 is amended: the prelude declares `String` as well as `Array`.

The blocker was never the `String` itself: record 0031 answered that from the
other end, and a user's class inside a `${}` is settled too, found by a
**`to_string`** convention.

**`T&&` does not parse**, a non-literal array length inside a **type** needs
constant evaluation, and the whole of track 3 (the `.hdm` blob) is untouched.

## Read this first next session

**A generic method is instantiated per call**, since 2026-09-09 — record 0055,
and it is record 0054's other half. `b.take<i32>(3)` did not **parse**: a type
argument list was read after a bare name and never after a `.`. That branch is
one line, shared with the bare name in `Parser::parse_generic_name`, and behind
it were four holes worth knowing about before touching any of this:

- **A method's clone lives in its class's body.** `Instantiator::instantiate`
  appended every clone to the module root, which for a method makes a free
  function: it loses the `this` its body reads and the call looks for a member
  the class does not have. The emitter wrote `b.class(3)` — `class`, because
  the node it fell back to carries token 0 and token 0 is the first word of the
  file.
- **The statement checker keeps its walked set by declaration, and a method
  clone does not grow at the root.** It grows inside a class the phase has
  already walked, so nothing typed its body. `has_more` and the round ask about
  members now. **Only a call the statement checker sees needs this** — a call
  inside a `let` is made while the type phase walks — which is why the first
  version of the test case did not prove it and the case writes one inside an
  `if`.
- **A clone is named in C++ by the name record 0002 already made unique.** A
  method is named by its own name and its parameters (record 0020, so an
  override carries one C++ name) and a clone copies the declaration's token, so
  `first<i32>` and `first<i64>` were **one** name for two methods differing
  only in what they give back. g++ refuses that. The name comes from the symbol
  table now — `first#7` — and only for a **method**: a free function was unique
  already and record 0054's output is byte for byte what it was.
- **A generic method nobody calls is not emitted**, for the reason a generic
  class is not. `emit_bodies` walked a class's members without asking.

A list written where there is no call — `b.take<i32>`, `b.n<i32>` — is refused
by name. One written on a call to something that is **not** generic is silently
ignored, for a method and a free function alike; changing that is one decision
about both.

**A generic may derive from a generic**, since the same day — record 0056, and
it is worth reading before touching the type phase, because what stopped it was
**one character**:

```cpp
if (candidate <= done || candidate >= count) {   // '<=' skipped one per round
```

`done` is the last index the pass finished, so the first **new** candidate sits
at `done` itself and every round skipped it. A **class** cloned from a generic
is the declaration that lands there, and `super_of` runs nowhere else — so its
`Candidate::super` was never built and stayed `INVALID_TYPE`, which the rest of
the compiler reads as *no base*. That was the whole of *cannot find 'value' in
this scope* and *expected Base<i32>*, found Derived<i32>*.

A refusal was decided and **withdrawn the same day** on a cost estimate that
was never measured. The record keeps both halves, and the rule: measure the
mechanism before deciding it cannot carry a feature.

Fixing the guard uncovered one thing worth knowing: a refused `for x in t`
**built its lowering anyway**, declaring `let __c0 = t.iterator()` whose
expression the collector then typed a second time. It read as one error only
because that local was the candidate the off-by-one skipped.
`ForEachLowerer::over_a_cursor` refuses now and puts the container's sibling
back.

**`def init : i32` is reported**, since the same day — agenda 5.6 closed, in
the OverrideChecker, which is the phase that already reads a signature and
reports about a declaration. It answers whether it reported so the override
half is not asked about the same method, and one mistake reads as one error.

**A construction is a call whose callee names a type**, since 2026-09-09 —
record 0045, and four things are worth knowing before touching it:

- **It is decided in the type phase and nowhere else.** `ExpressionTyper::call`
  asks `TypeBuilder::type_symbol` about the callee's candidates *before* it
  ranks, because a class answers to no signature and scored −1 against every
  argument — which is why `String("abc")` was *no 'String' takes these
  arguments*. The callee's two parts go to `TypeBuilder::build_written_name`,
  which is `build_named`'s body reached without an `AST_NAMED_TYPE` node, so a
  written generic instantiates for free.
- **The temporary is not hoisted, and that is deliberate.** Records 0032 and
  0037 hoist, and hoisting is exactly why a bracketed literal still reaches
  only a binding. The emitter writes `String(p)` in place — the same text it
  was already writing when it decided a conversion on its own — and C++17's
  guaranteed elision means even an uncopyable class constructs here.
- **`i32(x)` cost the parser one branch.** A builtin is a keyword and could not
  stand where a callee goes. It is read as the `AST_BUILTIN_TYPE` node it is
  and the postfix rule wraps it into a call — **not** into an `AST_CAST`, which
  would print back as `x as i32` and break the printer's round trip.
- **`new Box<i32>(7)` had never worked**, since generics landed, and nothing
  had written it. A clone made in the module being walked has no signatures
  yet and `catch_up` steps aside for precisely that reason, so an `init` read
  `INVALID_TYPE` and scored −1. `TypeCollector::type_signature_now` builds the
  one signature on demand; it is a request and not a pass, and safe to repeat
  because `signature_of` is pure.

**A conversion into a class is a constructor the compiler calls**, since
2026-09-09 — record 0046, which amends 0045 the same day. Record 0023's two
**named** entries are gone for good, and so is `Coercion::is_string`, which
compared a declaration's name to the text `"String"`. What stands in their
place knows no name:

```cpp
if (builds_from(module, wanted, given)) {
    return to->kind == TYPE_REFERENCE ? 2 : 1;
}
```

Four things to know before touching it:

- **It is on the list, so all four places ask it.** A call, a return, a
  binding and an assignment reach `Coercion::steps`. That is why
  `OverloadResolver::match` lost its special case for a written literal and is
  one line again — a literal and a value are the same question now, and the
  node kind stopped mattering.
- **The two costs are load-bearing.** By value one, by reference two, which is
  agenda 1.21's pair kept. With both `takes(String)` and `takes(String&)` in
  view a `char*` picks the by-value one, exactly as C++ does. Making them
  equal was tried and reports *this call matches more than one 'takes'
  equally well*.
- **The emitter needed nothing.** The tail of `emit_conversion` already writes
  `Class(expr)` from the two types. `by_value(plain)` and
  `by_value(String(plain))` emit byte-identical C++, which is the case
  `tests/emitter/cases/a_char_pointer_reaching_a_string` now shows.
- **This is C++'s converting constructor and there is no `explicit`.** Every
  one-argument `init` is an implicit conversion into its class and nothing can
  opt out. The argument for the asymmetry it replaced is kept in record 0046
  rather than deleted with the refusal it used to justify.

**Comparing two containers left the containers**, 2026-09-09 — record 0054.
`Array<T>` and `List<T>` no longer declare `equals`, `==` or `!=`, and both
carry a comment saying the absence is the decision. A method of a generic
class is instantiated **with the class**, so an `equals` comparing two `T`
made every `T` ever put in one need an `operator==` for a method nobody
called — which is the whole of why `Array<Token>` did not compile, and the
bootstrap is `Array<Token>`, `Array<AstNode>` and `Array<Type>`. Measured
first: **no program in this repository has ever compared two of them**.

**A generic function is instantiated per call**, since the same day, and it
had **always parsed and never been instantiated**: `callee_of` opened by
dropping the type arguments, so `f<i32>(3)` was ranked against the unbound
signature. The Instantiator could always have done it — it clones a
declaration and asks nothing about its kind. Three things to know:

- **`check_statements` is a loop now.** A call carrying type arguments
  instantiates, and a call is checked there as often as it is typed in the
  type phase, so a clone can be born after its module's turn. The checker
  keeps a set of the declarations it has walked, and reads the declaration
  list into a **snapshot** before walking — a clone is appended as it is made,
  so a live walk would reach it in the same round, before the type phase has
  been through its body.
- **`UseResolver::use()` writes no resolution.** It resolves a name only to
  check that it exists; the ResolutionTable is written by the type phase. An
  afternoon went into resolving a fresh clone's uses before that was measured,
  and the work was reverted rather than left in looking useful.
- **`catch_up` must not run the written pass over what inference finished.**
  It ran `walk(index, false)` over every candidate past the `collected` mark,
  and once inference has run that mark is behind — so a clone the inferred
  pass had already finished was re-typed with what it **wrote**, and what a
  `let i = 0` writes is nothing. `Compilation::collect_types` carries that
  exact sentence about why its two loops are not nested; this is the same trap
  from the other side, and it only appeared because a call with type arguments
  instantiates from the **statement checker**, where no walk is running and
  `catch_up` really walks. After inference, it uses `walk(index, true)`, which
  gives a declaration both passes in order.

**`main` takes its arguments**, since 2026-09-09 — record 0051, in two shapes
besides the empty one: C's `(argc, argv)`, and **one list**, `@args :
String[]`, built in the shim out of `argc` and `argv`. The list form knows
**no library class** — it asks the parameter's class for `add` by name, the
way record 0040 asks a container for `iterator` — so a reader's own two
classes work exactly as `Array<String>` does. It found one thing worth
keeping: **the shim is the one call in a program that no Haard source wrote**,
so it is also the one place that has to ask record 0031's question itself. A
first draft handed a list that owns memory over by value and the result was a
double free g++ wrote without a word.

**`super(...)` gives a base its arguments**, since 2026-09-09 — record 0053,
and before it a class whose every `init` took an argument could not be derived
from at all. Three things to know:

- **It may be written anywhere in an `init`**, not only first — refusing
  otherwise would be the compiler guessing at intent. But **every** `init` of
  the class must write one, and a class that declares none is still refused,
  because there is nowhere to put it.
- **A class whose `init`s all take arguments now gets a do-nothing C++ default
  constructor**, which is what the derived class's base sub-object binds to
  before `super` fills it in. Getting that right needed *takes no parameters*
  to become **answers to no arguments** — record 0012's arity-as-a-range — or
  an `init(@count : i32 = 3)` got a second empty constructor beside it and the
  two were ambiguous, deleting the implicit default of every class derived
  from it.
- **`holder_of` gives back a declaration's node and `name_of` wants its
  candidate.** Handing the node over named whatever candidate sat at that
  index, so the derived `init` called itself and the program overflowed its
  stack. The emitter suite caught it as a segmentation fault.

**A range is a value**, since 2026-09-09 — record 0052. `0..10` was the last
expression kind that typed to nothing, and it is a **`Range<T>`** now:
`std/range.hd`, start, stop and step, with `stop` one past the end the way
Python's is. **Inside a `for ... in` none of that happens** — record 0040
reads the node as syntax and writes a plain loop, so walking a range still
allocates nothing.

It found a bug with nothing to do with ranges: **a generic cloned during
inference, in the module that declared it, was never given its own type**, so
`this` inside the clone's own methods came out as `<none>*`. From another
module it never showed, because the walk had already run, and a written type
never showed either, because that clone is made during collection. It needed a
generic declared and instantiated in one module and inferred rather than
written, and nobody had done that. `TypeCollector::type_signature_now` types a
class candidate too now — its own type and its methods' signatures — and
`TypeBuilder` calls it at **both** places an instantiation happens.

**`isize` and `usize` exist**, since 2026-09-09 — record 0050, and they emit
`intptr_t` and `uintptr_t`. The signed one is **not** `size`: that was asked
for and measured first, and `size` is the field name of `String`, `List`,
`Hash` and `Array` — **727 uses** here, and every program with such a field
would have stopped parsing. Two things to know before adding another builtin,
because both bit:

- **Three places asked *is this a whole number* positionally**, by writing
  `which <= BUILTIN_I64`. Two integers added after `i64` would have been
  silently classified as not-integers. They are one function now,
  `is_a_whole_number`, next to the enum.
- **`tests/type_table/types.cpp` and `tests/resolution_table/resolutions.cpp`
  keep their own copy of the builtin name table**, indexed by the enum and
  built from nothing. The whole type_table suite failed reporting `f64` as
  `void`. They are still copies; a third would break the same way.

Two goldens moved for reasons with no meaning in them, both checked line by
line: type_table counts **interned types**, up two everywhere, and the
emitter's **mangled names** encode the builtin index, so `m_poke_b12` became
`m_poke_b14`.

**`as` is a closed list**, since 2026-09-09 — record 0049, and until that day
it checked **nothing**. `ExpressionTyper::cast` typed its operand for the
record's sake and gave back the written type, so `pt as i32` over a class
passed `hdc` and died in g++ with a mangled name, about a line nobody wrote.
The list fits in a sentence: **a number to a number, a pointer to a pointer, a
pointer and a whole number it fits in, up or down a chain of bases through a
pointer or a reference, and a `symbol` to a `char*`.** Four things to know:

- **It was measured, not imagined.** The typer was instrumented to dump every
  `(from, to)` pair and run over `examples/`, all thirteen test programs and
  every case of nine suites: **21 distinct pairs**, twenty of them between
  numeric builtins. Nothing that compiled before stopped compiling.
- **A class by value is never reached by a cast**, even up a chain it really
  is on — that is slicing. Through a pointer or a reference it is allowed.
- **Sixty-four bits is assumed.** A pointer converts only to `i64` or `u64`,
  because `p as i32` was on the list for an afternoon and g++ answered *loses
  precision*. Haard has no target model, so `holds_a_pointer` is the first
  line to change the day it emits for something narrower.
- **Record 0045's spelling asks the same list.** `i32(p)` and `p as i32` are
  one conversion written two ways and emit the same C++, so leaving the
  construction branch unchecked would have made the list a suggestion. Both
  are pinned side by side in
  `tests/type_table/cases/a_cast_is_a_closed_list`.

**`print` and `println` cover every builtin**, since 2026-09-09. They covered
`i32`, `i64`, `u32` and `f64` and nothing else, so a program holding an `i8`
could not print one. Six were added — `i8`, `i16`, `u8`, `u16`, `u64`, `f32` —
and the same six went into `String.append`, which is the layer `${}` reaches
and where the hole really was. **`u64` has its own digits**: the other five
widen and hand over, and past `i64`'s largest that would come out negative,
which is what the golden of `printing_is_a_free_function` shows.

**Read record 0047 before proposing that the compiler refuse something.** It
decides no mechanism: it writes down the **stance**, after a conversation in
which four programs that Haard compiles and C++ refuses were each put to
Hadley and each kept. The rule is *the compiler refuses when it cannot decide,
not when it disapproves* — so a write lost through a temporary, a dangling
reference given back from a `T&`, and a raw `i8*` becoming a `File` are the
author's to avoid, while an ambiguous call is refused because *I cannot tell
which* is a fact about the compiler rather than a guess about intent.

**Agenda 1.37 — which *pairs* of types may convert — was opened and closed the
same day**, record 0048, and the decision is that **nothing changes**: the
builtin half of record 0018's list stays empty, a one-argument `init` stays a
conversion, and **no marker is added**. There is no code for it; the record
exists so the next person to notice that an `i8*` becomes a `File` finds an
answer instead of an oversight.

Three things it measured are worth keeping:

- **The whole standard library reaches exactly two class conversions** —
  `char*` → `String`, intended, and `i8*` → `File`, the accident. Every other
  one-argument `init` is unreachable: `builds_from` needs the parameter type
  **exactly**, and `Coercion::steps` reads a reference *before* asking it, so
  `List<T>`'s `init(@from : Array<T>&)` can never be reached from an
  `Array<i32>&`.
- **Two of sixteen casts** in `std/` would have disappeared under lossless
  widening. The rest are narrowing, sign changes, `symbol` → `char*`, or
  `i64` → `f64`, which is widening by the letter and lossy in fact.
- **The reversibility is asymmetric**, and this is the part a later session
  needs: lossless widening and an opt-out `explicit` both break nothing if
  added later, while an opt-in marker breaks every unmarked conversion and
  gets more expensive with every class written. Two doors stay open for free;
  the third narrows.

**The standard library moved, on 2026-09-08, and Hadley asked for it.**
`std.io` is now `print` and `println` — free functions, overloaded on `char*`,
`String&`, `char`, `i32`, `i64`, `u32`, `f64`, `bool` and `symbol` — and the
eight natives moved to **`std.low_io`**. A program prints with `import std.io`
and `println("hello, world!")`, and never with `let out = console()` first.
Three things to know before touching it:

- The emitter matches **one string**: `Emitter::native_body`'s
  `module->get_name() != "std.low_io"`. That is the whole of the rename in the
  compiler, and pointing it back at `"std.io"` breaks
  `tests/emitter/cases/input_and_output` immediately.
- **`print` stands on the natives and not on `File`**, because `console()`
  builds a `File` with `new` and hands back the pointer — a `print` written
  over it would leak one per call. `std.file` is unchanged and puts the same
  names on a `File`.
- **The library is at the repository root, in `std/`**, and every test case
  that needs it carries its **own copy** under `cases/<name>/std/`. Several of
  those copies are deliberately **trimmed** — an older `String` with no
  `operator==`, a `File` with no `String` overloads — so a change to `std/`
  is not a `cp` over all of them. `tests/programs/cases/*/std` and
  `tests/emitter/cases/input_and_output/std` are the ones that hold the real
  natives; the `std/io.hd` files under `tests/name_resolver`,
  `tests/module_finder` and `tests/compilation` are unrelated toys.

**Two enums compare with `__equals`**, written on the struct by the emitter:
the tag first, then the payload, and a class payload by the `operator==` it
wrote. A class that wrote none leaves the enum **without** `__equals` — the
enum is still built, matched and destroyed like any other, and only comparing
two of them is refused, at the **comparison** and by name. That is
`Emitter::union_compares`, and the two cases that pin it are
`an_enum_that_carries_a_class` (emits, no `__equals`) and
`an_enum_that_cannot_be_compared` (the refusal is the golden).

**The README is current**, rewritten 2026-09-08, and every snippet in it was
compiled and run before it was written. It is the best short tour of what the
language does today; `examples/hello` and `examples/shapes` are in it, both
runnable, and `examples/shapes` is a program made of an app and two libraries
that find each other through their manifests.

**`char*` → `String` is on record 0037's mechanism** for a written literal,
since 2026-09-08. Two things to know before touching it: the **ranking** is
still record 0023's (a literal into a constructible class costs one step, in
`OverloadResolver::match`, which is what keeps `char*` ahead of `String`), and
the **bracketed** literals still reach only a binding, because record 0037's
own hoisting turns them into a name before a call, a return or an assignment
is reached. A string literal is not hoisted, which is why it could move.

**The first of the bootstrap's five blockers is gone.** Enums reach the emitter
now (record 0043), which was 1,934 uses and the largest of them. What is left
is `main(argc, argv)`, the file system, and whether a generic instantiates a
method nobody calls.

**Can this compiler be written in Haard yet?** Asked and answered on
2026-09-08, measured against `src/`:
`design/notes/bootstrapping-haard-in-haard.md`. **Not yet, and it is five
things**: enums that reach the emitter (1,934 uses, and a member access types
to nothing today), `main(argc, argv)`, the file system (62 uses of
`std::filesystem` against eight natives), ~~`&xs[i]`~~ (fixed the same day) and
**whether a generic instantiates a method nobody calls** — which is the one
that needs a decision, since `Array<Token>` fails on an `operator==` that
nothing called. The scanner is where a bootstrap starts, because its 45 goldens
check text and do not care which language produced it.

**The next thing to build**: nothing in the standard library is missing any
more — `Hash<K, V>` landed on 2026-09-08 (record 0042) and record 0022's four
classes are all written. What is left of the library is `{key: value}` (agenda
1.23, deferred with its design written down) and a **closure that types**,
which is what an `each` or a `map` waits on.

**Two things writing the Hash found**, and both are worth knowing before
touching the type phase:

- **A call inside a generic nobody instantiated says nothing when it matches
  nothing.** `hash_of(key)` over an unbound `K` matched no overload and was
  reported about a class that is fine. The statement checker and the emitter
  already skip an unbound generic whole; the type phase now skips this one
  thing, and the clone reports it properly.
- **An operator's operand is an argument.** `s == "abc"` handed a raw `char*`
  to an `operator==` taking a `String&` and **g++ refused it** — the copy
  assignment's bug of the same morning, one method over. `emit_operator` runs
  its operand through `emit_conversion` now.

**`for x in` is built**, since 2026-09-08 — record 0040. All three shapes run:
a class walked by the **cursor** it gives back (`iterator()`, `has_next()`,
`next()`, one cursor class per container), a **fixed array** walked by the
length its type carries, and a **range**, which is a plain loop and no class.
`x` is a **reference**, so writing it writes the element.

Five things about it are worth knowing before touching it:

- **It is not a phase in `Compilation::build`.** A pass after the type phase is
  too late: `for x in xs: let y = x` infers `y` once and never again, so the
  loop has to be taken apart **before the body's own bindings are inferred**.
  It is driven by the **loop variable's candidate** instead — `SymbolCollector`
  points it at the loop, `TypeCollector::type_of` sees that and calls
  `ForEachLowerer`, and candidates are walked in source order, which is exactly
  the order this needs.
- **The loop becomes a block**, rewritten in place, holding the cursor and the
  loop. The emitter gives that block **braces**, which is what lets two loops
  in one function declare two `x`es — and the block keeps the scope the
  collector opened for the loop, because a scope is stamped with its node.
- **The range and the fixed array lower to a C shaped `for`** and not to a
  `while` with the step at the end of the body, which a `continue` would jump
  over.
- **A range walks with the type of its end**, so `for i in 0...xs.length()`
  over a `u32` walks with a `u32`.
- **`..` takes its end in and `...` stops before it** — the scanner's rule,
  Ruby's convention, and the opposite of what record 0040's own examples were
  written with. Record 0040 says where it would change.

What it refuses, each exactly once: `for key, value in pairs` (taking a value
apart is 1.23's question), `for c not in xs`, a sequence that is not one of the
three shapes, and a class answering none of the three names — that last one
reported by the **call**, which says what to write.

**`[1, 2, 3]`, `{1, 2, 3}` and `List<T>` all work**, record 0037, and the
shape to know is that they are one thing: a `{}` is a C++ array, a `[]` is that
plus one constructor call, and a written class type is that plus one more. No
`add` loop anywhere.

Three things about it are worth knowing before touching it:

- **The statics are spliced above the globals**, not merely above the bodies. A
  global literal's array came out after the line that named it.
- **Where a constructor is resolved depends on who wrote the type.** A written
  `List<i32>` was instantiated during collection, so the typer can ask. A bare
  `[1,2,3]`'s `Array<i32>` is cloned during **inference** and its signatures
  arrive later, so the **emitter** asks instead.
- **Only the statement checker skips an unbound generic**, not the type
  collector. Making the collector skip them too was tried, changed three
  goldens and fixed nothing.

**`Array<T>` runs, `T[]` is sugar for it, and operators are overloadable.**
That is what changed on 2026-09-06, and the three of them are one piece of
work: `xs[0]` needs an operator, an operator giving back a `T&` needs a
reference to read as a value, and `T[]` needs the prelude to name `Array`.

**Four holes were found by writing the `Array`**, and every one of them was
silent or in the wrong place:

- a `T&` could not be read as a value **anywhere** — six places (record 0035)
- a generic **naming itself** could not be emitted, which every container that
  owns memory does, because record 0031 makes it write `copy` (record 0036)
- a class with `copy` and no `init` lost C++'s implicit default constructor, so
  it compiled in Haard and failed in **C++** (record 0036)
- two class values compared with a builtin operator passed the type phase in
  **silence** and came out as C++ that does not exist (record 0034)

That is the fifth session running that the pattern has held: write ordinary
Haard, run it, find what every suite was green through.

**One recorded behaviour changed**, and Hadley approved it: assigning to a
reference writes through it. `tests/statement_checker/cases/the_coercion_list_is_not_only_for_a_call`
says why in its own comment.

**What is next is `[1, 2, 3]`.** Hadley decided it **infers** as an `Array<T>`
and is a `List<T>` only when explicitly typed, with `List<T>` taking an
`Array<T>` in its constructor. Neither is built. The literal is still a
`TYPE_LIST`.

**Template strings work.** A template string becomes a local `String` and a run
of `append` calls, hoisted before the statement it was written in, in
`src/haard/sugar/sugar_lowerer.{h,cpp}` — the Ast → Ast pass record 0025 named
and nothing had needed until now. It runs **before the symbols are collected**,
because it writes a declaration, and nothing after it learned that a template
string exists: each `${}` finds its `append` by its own type, which is what
makes String's eight overloads carry the whole feature.

Three things about it are worth knowing before touching it:

- **A synthetic token.** `Module::get_token_value` slices the source by an
  offset, so `String`, `append` and `__tsN` have nowhere to live. Their text is
  kept beside the stream, and the offset is still used for **where to point** —
  each is given a real token's position, so a diagnostic about a node nobody
  wrote lands on the `${}` the author did write.
- **A refused one is still a `String`**, with the appends dropped. Without that
  recovery a refusal reads as two errors, the second a consequence of the
  first.
- **`tests/sugar/` prints the tree back as Haard.** The golden *is* the sugar
  taken apart, which is record 0025's argument for a pass over an IR being
  cashed in.

Three places are refused and loosening them is additive: an operand of
`and`/`or`, a loop condition, and module level.

**The prelude is in, and it is not the one record 0017 designed.** That is what
changed on 2026-09-06, and record **0033** supersedes 0017's mechanism.

0017 could not have worked as written, and the reason was found one design
session before it was built: an import is not transitive, and
`NameResolver::module_symbol` looks only in a dependency's **own** module
scope — so a prelude module that wrote `import std.string` would have given
`String` to nobody, and the whole standard library would have had to move
inside the prelude.

Hadley's answer was to drop the module: the table carries a **list of imports**
every module is given. Two things were settled while building it that the idea
did not say — **the Ast is never touched** (the dependency list is all a lookup
reads, and a synthetic import node has no source position and would print), and
**the list is resolved once against the table** (an import resolves against the
importing root's visibility list, so per-module resolution would fail in every
root that does not name the library).

Ten cases, and every one proven by sabotage. The two guards are the ones to
know about: a module of the prelude importing **itself**, and a module that
**also wrote** the import — and the second is not cosmetic, because
`NameResolver::gather` appends without de-duplicating, so a duplicate
dependency is a **false ambiguity** at a call.

**Template strings are the next thing**, and nothing is in the way now.

**Template strings are half a feature, and the missing half has a design
and an order.** That was 2026-09-06 earlier the same day. The front end has been
complete since the 5th; nothing after it knows the node exists, so a template
string passes `hdc file.hd` in silence. Record 0032 says how it lowers — a
hoisted local `String` and a run of `append` calls, refused where hoisting
would change the meaning — and Hadley chose the **prelude** to give the pass a
`String` it can name. So **the prelude is now item 2 and template strings are
item 3**, and record 0017's open point 4 is amended: the prelude declares
`String` as well as `Array`.

Nothing was implemented for it. `git status` should show only the four
documents.

**Generics run.** That is what changed on 2026-09-05. A generic class written
with arguments becomes a concrete class, monomorphised in the front end, and
the whole program still comes out as one C++ file with no `template` in it.
Record 0002, made real by `src/haard/type_table/instantiator.{h,cpp}`.

**A bare name on the left of `=` now sees a base**, record 0027. `n = 7` in a
method of a derived class used to declare a **local** in front of the inherited
field — the field never written, the local never read, nothing said. The
decision moved out of the symbol phase, where a base is not yet a type, into
the type phase and a new `ImplicitCollector`. And a derived class that declares
a field a base already declares is now an **error**.

`make check` is green: 15 suites, ~620 cases, about 40 seconds. The emitter suite's golden
ends in the **exit status of the binary it built**, which is what makes it the
one suite that cannot be satisfied by output that merely looks right.

**`docs/` is still line 66 of `.gitignore`.** Every record in this directory
exists only on disk, with no history and no copy.

**Nine silent bugs have now been found the same way — by writing ordinary
Haard and running it, every one with `make check` green.** The list is under
*What does not work* above, and all nine are fixed. The pattern is the point,
and it is the fourth session running that it has held.

The ninth is worth reading on its own, because it is the first one a **test**
could not have caught by being more thorough about the same shape: it needed a
shape nobody had written, and the compiler's own two phases disagreed about it
in the meantime.

## What to do next, in order

0. ~~The C++ emitter~~, ~~`init` and `destroy`~~ and ~~generics~~ — **done**,
   records 0025, 0026 and 0002. What the emitter still refuses it says so
   about, by name.

0. ~~`main(argc, argv)`~~ — **done 2026-09-09**, record 0051, and in two
   shapes. ~~`super`~~ — **done**, record 0053. ~~A range that types~~ —
   **done**, record 0052. ~~A generic **method** call~~ — **done**, record
   0055. ~~`class Derived<T>(Base<T>)`~~ — **works**, record 0056, and the
   blocker was an off-by-one and not the design. ~~`def init : i32`~~ —
   **reported**, agenda 5.6.

   What is left in the front end, in order:

   1. **A closure that types** — the biggest by far, and the only one that
      needs a decision before any code. `AST_CLOSURE` is in the parser, the
      builder, the printer and the symbol collector, and the statement checker
      only says *I do not know what this gives back*. **Nothing in
      `type_table/` has ever seen one.** `each` and `map` wait on it.
   2. **A generic called without written type arguments** — `f(3)` and
      `b.take(3)` alike, which need the parameter types unified against the
      argument types. There is no unification in the compiler at all. One
      subject for the function and the method, since record 0055 made them one
      mechanism.
   3. The small ones: `T&&` does not parse (1.15), record 0018's list does not
      compose (`char*` → `String&`), the three template-string refusals are
      loosenable and additive, a constant inside a **type** cannot be
      evaluated, and a **tuple as a value** types but does not emit.
   4. Deferred on purpose, with the design written down: 1.23 (`{key: value}`)
      and 1.26 (`const`). Do not re-derive either.

1. **More real Haard.** `tests/programs/` is the newest suite and the only one
   that runs the flow a **user** runs: the real `hdc` binary, through each
   case's own `Makefile`, to a binary that checks its own work. Two cases so
   far — a PPM reader and writer, and a drawing library over it with lines,
   rectangles, circles, Béziers and bitmap text, all integer arithmetic. Its
   goldens hold the **picture**, in `#` and `.`. Between them they found three
   compiler bugs in a day. More programs there is the highest-value thing to
   do.
2. ~~**The prelude**~~ — **done**, record 0033, and it is a list of imports in
   the table and not a module. `import std.io` stops being necessary by being
   a line in that list. What it does **not** do yet is `T[]` → `Array<T>` and
   a type for a string literal: those need an `Array` to exist, which is item
   4, and the sugar to be pointed at it.
3. ~~**Template strings**~~ — **done**, record 0032. What is left is
   **loosening** the three refusals, which is additive and needs no decision:
   an `and` lowers to the branch it already is, and a loop condition by
   rebuilding at the top of the body.
4. **The rest of the standard library** (records 0022 and 0023). `Array` is
   in (record 0036); `List` and `Hash` follow. What is next inside it, in
   order:

   - ~~`[1, 2, 3]` and `List<T>`~~ — **done**, record 0037.
   - ~~**`for x in <container>`**~~ — **done** 2026-09-08, record 0040, and
     `ArrayCursor<T>` and `ListCursor<T>` are in the library beside their
     containers. See *Read this first* above.
   - ~~**`Hash<K, V>`**~~ — **done 2026-09-08**, record 0042: open addressed,
     hashed by an overload set, walked by its keys, and in the prelude. What
     is left of the library is `{key: value}` (agenda 1.23, deferred with its
     design written down) and a **closure that types**, which is what `each`
     and `map` wait on.
   - ~~**Migrating `char*` → `String` onto record 0037's mechanism**~~ —
     **done 2026-09-08** for a written **literal**, and **finished
     2026-09-09** by records 0045 and 0046 together: `T(args)` is writable
     (which closes the gap record 0040 also ran into), the named entries are
     deleted, and a **value** converts by an entry that asks the class for a
     constructor instead of reading its name. A literal and a value are one
     question now.
   - **Whether record 0018's list composes**, which record 0035 left open:
     `char*` → `String&` needs two entries and gets none. It is the only
     question left on that list, now that the library entries are gone.
5. Agenda **5.6** — `def init : i32`, a constructor that gives something back,
   which nothing reports. One check, and it belongs with the override
   checker's family. **5.5 is closed** (record 0028). And **1.23** (what a
   brace literal is) and **1.26** (`const`) are deferred on purpose, both with
   their design written down.

## The lesson of 2026-09-02, worth keeping

Three blocking bugs were found in one day, all by **writing ordinary Haard and
running it**, and all while every suite was green:

- a derived class could not override a method — it was an ambiguous overload
- an inherited member was invisible to a bare name inside a method
- fourteen expression kinds typed to nothing

The goldens prove what the cases write, and nobody had written an array
subscript. `tests/type_table/cases/every_expression_kind` exists so that "no
one has written this yet" shows up as a visible `<none>` in a golden instead of
as silence.
