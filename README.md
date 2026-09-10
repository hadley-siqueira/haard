*For God so loved the world, that he gave his only begotten Son, that*
*whosoever believeth in him should not perish, but have everlasting life.*

*For God sent not his Son into the world to condemn the world; but that*
*the world through him might be saved.*

*John 3:16-17*

# The Haard Programming Language

Haard is a programming language with syntax inspired mainly by Python, but with
semantics closer to C++. It has classes, single inheritance, generics,
operators, enums that are tagged unions, pattern matching, type inference,
pointers and manual memory management.

This repository holds `hdc`, the first compiler of the language, written in
C++. **`hdc` transpiles a whole Haard program into one C++17 file**, which a
C++ compiler then turns into a binary. It runs today: hello world, programs of
several modules and libraries, a PPM image writer, a drawing library, a binary
tree over an enum, and the language's own standard library — `Array<T>`,
`List<T>`, `Hash<K, V>`, `String`, `File` and `print`/`println` — are all
written in Haard and all run.

There is no intermediate representation and no build system inside the
compiler: `hdc` is told where a program's libraries are and searches nowhere
else.

## Install from source

```
git clone https://github.com/hadley-siqueira/haard.git
cd haard
mkdir build
cd build
cmake ..
make
```

There will be a binary named `hdc` in the `build` folder. Nothing outside a
C++17 compiler, `cmake` and `make` is needed — the project has no dependencies.

```
usage: hdc [options] <file.hd>

options:
  -h, --help          show this message
      --tokens        dump the token stream
      --emit-cpp      write the program as C++, for a C++ compiler
      --pretty-print  print the source back from the ast
      --pkg <file>    the manifest ('haard.pkg'), which says what
                      this library is and what it needs. hdc
                      reads it and the manifests it reaches.
                      Without it, the nearest one at or above the
                      input file is used
      --roots <file>  the roots table. Without it the imports of
                      the input file are not followed
```

## Hello, world

A Haard program is a directory with a `haard.pkg` beside its sources. That file
is **the only one written by hand**: it says what this program is called and
what it needs. It is a Haard literal, read by the language's own parser, so a
typo in it is a diagnostic with a caret like any other.

`examples/hello/haard.pkg`:

```haard
{
    name: "hello",
    version: "0.1",

    dependencies: [
        {name: "std", path: "../../std"}
    ]
}
```

`examples/hello/main.hd`:

```haard
import std.io

def main : i32
    println("hello, world!")

    return 0
```

`print` and `println` are ordinary functions in `std.io`, overloaded on what
they are given — a `char*`, a `String`, an `i32`, a `char`, a `bool`, an `f64`,
a symbol. Nothing is opened and nothing is constructed first. Writing to a file
is `std.file`, which puts the same names on a `File`.

Build and run it:

```
cd examples/hello
../../build/hdc --emit-cpp main.hd > hello.cpp
g++ -std=c++17 -w -o hello hello.cpp
./hello
```

```
hello, world!
```

`hdc` was given no flags at all: with no `--pkg` and no `--roots` it looks for
the nearest `haard.pkg` at or above the file it was handed, reads it, follows
its dependencies to `std/haard.pkg` and compiles the program those manifests
describe. Saying it out loud is the same thing:

```
../../build/hdc --pkg haard.pkg --emit-cpp main.hd > hello.cpp
```

### What comes out

The whole program — the entry file, the standard library it reached, and the
handful of native functions that touch C's stdio — arrives as one C++ file of
438 lines. Its end:

```cpp
int32_t h0_1_main() {
    h1_27_println("hello, world!");
    return 0;
}

int main() {
    return h0_1_main();
}
```

Names are mangled by the module they came from and by their parameters, which
is what makes overloading and overriding work across modules. The `-w` above
only silences g++'s warning about string literals being `char*`.

## A project with two libraries

`examples/shapes` is a program made of three packages: an application and two
libraries it includes, one of which depends on the other. Each is a directory
with its own `haard.pkg`.

```
examples/shapes/
    app/        haard.pkg   main.hd
    geometry/   haard.pkg   shape.hd
    report/     haard.pkg   lines.hd
std/
    haard.pkg   array.hd  string.hd  list.hd  hash.hd
                io.hd     file.hd    low_io.hd
```

**`app/haard.pkg`** — what the program needs, and the name it will import each
one by:

```haard
{
    name: "app",
    version: "0.1",

    dependencies: [
        {name: "std", path: "../../../std"},
        {name: "geometry", path: "../geometry"},
        {name: "report", path: "../report"}
    ],

    prelude: ["std.array", "std.string", "std.io"]
}
```

`prelude` is the list of imports **every module of this program** is given, so
nothing below writes `import std.io` — it is how a program says which names
should always be in view. Only the program's own manifest is asked for one: a
library deciding that for its importer would be a library deciding too much.

**`geometry/haard.pkg`** — a library that needs nothing:

```haard
{
    name: "geometry",
    version: "0.1"
}
```

**`report/haard.pkg`** — a library that needs another one, and says so itself.
The app never had to know:

```haard
{
    name: "report",
    version: "0.1",

    dependencies: [
        {name: "std", path: "../../../std"},
        {name: "geometry", path: "../geometry"}
    ]
}
```

**`geometry/shape.hd`** — a shape is one of three things and never anything
else:

```haard
enum Shape:
    Point
    Circle : i32
    Rect : (i32, i32)

def area_of : i32
    @s : Shape

    switch s:
        case Point:
            return 0

        case Circle(r):
            # 3 is close enough to pi for an example
            return 3 * r * r

        case Rect(w, h):
            return w * h
```

**`report/lines.hd`** — the same enum, read by another library:

```haard
import std.string
import geometry.shape

def described : String
    @s : Shape

    switch s:
        case Point:
            return "a point, with no area"

        case Circle(r):
            let out : String = "a circle of radius ${r}, area ${area_of(s)}"

            return out

        case Rect(w, h):
            let out : String = "a ${w}x${h} rectangle, area ${area_of(s)}"

            return out
```

**`app/main.hd`**:

```haard
import geometry.shape
import report.lines

def main : i32
    let shapes : Shape[] = [Point, Circle(2), Rect(3, 4)]
    let total = 0

    for s in shapes:
        println(described(s))
        total = total + area_of(s)

    println("total area: ${total}")

    return 0
```

```
cd examples/shapes/app
../../../build/hdc --emit-cpp main.hd > shapes.cpp
g++ -std=c++17 -w -o shapes shapes.cpp
./shapes
```

```
a point, with no area
a circle of radius 2, area 12
a 3x4 rectangle, area 12
total area: 24
```

Each library's manifest is read where it lives, and a path is always resolved
against the manifest that wrote it — so the same command run from anywhere
resolves to the same files. A library reached twice is one library, and two
libraries may carry the same name without colliding, because the name in
`dependencies` is the name **the importer** chose.

### What comes out

The enum becomes a struct with a tag and a union, plus one maker per variant:

```cpp
struct h1_1_Shape {
    int32_t tag;
    union {
        struct { int32_t _0; } h1_3_Circle;
        struct { int32_t _0; int32_t _1; } h1_4_Rect;
    };

    bool __equals(const h1_1_Shape& other) const { /* tag, then payload */ }
};

h1_1_Shape h1_1_Shape_h1_3_Circle(int32_t a0) {
    h1_1_Shape made;
    made.tag = 1;
    made.h1_3_Circle._0 = a0;
    return made;
}
```

The `switch` becomes a C++ switch on that tag, and a capture becomes a
**reference into the value being matched** — taking a payload apart copies
nothing:

```cpp
int32_t h1_5_area_of(h1_1_Shape h1_6_s) {
    {
        h1_1_Shape &__sw1 = h1_6_s;
        switch (__sw1.tag) {
            case 0:
            {
                return 0;
                break;
            }
            case 1:
            {
                int32_t &h1_7_r = __sw1.h1_3_Circle._0;
                return 3 * h1_7_r * h1_7_r;
                break;
            }
            ...
        }
    }
}
```

And `main`, with the array literal, the `for ... in` and the template string
all taken apart:

```cpp
int32_t h0_1_main() {
    h1_1_Shape __fx0[3] = {h1_1_Shape_h1_2_Point(), h1_1_Shape_h1_3_Circle(2),
                           h1_1_Shape_h1_4_Rect(3, 4)};
    h3_42_Array h0_2_shapes(__fx0, 3);
    int32_t h0_3_total = 0;
    {
        h3_75_ArrayCursor h0_6___c0 = h0_2_shapes.m_iterator();
        while (h0_6___c0.m_has_next()) {
            h1_1_Shape &h0_4_s = h0_6___c0.m_next();
            h5_31_println(const_cast<h4_1_String&>(
                static_cast<const h4_1_String&>(h2_1_described(h0_4_s))));
            h0_3_total = h0_3_total + h1_5_area_of(h0_4_s);
        }
    }
    h4_1_String h0_5___ts0;
    h0_5___ts0.m_append_pb12("total area: ");
    h0_5___ts0.m_append_b6(h0_3_total);
    h5_31_println(h0_5___ts0);
    return 0;
}
```

Note what is **not** there: no C++ template, no `operator+` and no `operator=`.
Generics are monomorphised in the front end — `Array<Shape>` is a class of its
own by the time C++ sees it — and an operator in Haard is a method with a name
no source can write, so the emitted code never leans on C++'s own overloading
to mean the right thing.

## The manifest

`haard.pkg` is data and never a program: a compiler that has to run code to
find out what to compile is not one anybody can trust. What it holds:

| key | what it is |
|---|---|
| `name` | what this library calls itself. Required |
| `version` | what it calls this version. Not resolved yet |
| `dependencies` | a list of records, each with a `name` and a `path` |
| `prelude` | imports every module of **the program** is given |
| `tool` | reserved for anything else that wants to keep settings here; `hdc` ignores it |

A dependency writes exactly one of `path` and `version` — never a microformat
like `"file:../x"` or `"^2.0.0"`. Only `path` resolves today; a dependency
asked for by version is refused by name, because there is nowhere yet for
versions to live. There is no registry, no solver and no lock file: those are
the parts every ecosystem regrets, and they can arrive the day versions do.

Only the **program's** prelude is read. A library saying what every module of
the program should be given would be a library deciding for its importer.

## A tour of the language

Everything below compiles and runs today.

### Functions

Parameters are declared one per line, `@name : type`. The syntax was inspired
by VHDL, where one input or output per line is the norm.

```haard
def sum : i32
    @a : i32
    @b : i32

    return a + b
```

Which is there because this, from glib, is awkward to read:

```c
GArray*
g_array_sized_new (gboolean zero_terminated,
                   gboolean clear,
                   guint    elt_size,
                   guint    reserved_size)
{
```

and this is not:

```haard
def g_array_sized_new : GArray*
    @zero_terminated : bool
    @clear           : bool
    @elt_size        : u32
    @reserved_size   : u32
```

Functions overload on their parameters, and a type is written once — `let` is
optional and inference does the rest.

### Control flow

```haard
import std.io

def main : i32
    let i = 0

    while i < 10:
        i = i + 1

    for j = 0; j < 10; j = j + 1:
        println(j)

    for k in 0 .. 3:          # 0, 1, 2, 3 -- inclusive
        println(k)

    for k in 0 ... 3:         # 0, 1, 2    -- exclusive
        println(k)

    if i > 5:
        println("big")
    elif i == 0:
        println("zero")
    else:
        println("small")

    return 0
```

`for x in` also walks anything that answers `iterator()`, `has_next()` and
`next()` — which is what the standard library's containers do — and a fixed
array, by the length its type carries. The loop variable is a **reference** to
the element, so writing to it writes into the container.

### Classes

```haard
class Point:
    x : i32
    y : i32

    def init : void
        x = 0
        y = 0

    def operator+ : Point
        @other : Point&

        let p : Point

        p.x = x + other.x
        p.y = y + other.y

        return p

    def to_line : String
        let out : String = "(${x}, ${y})"

        return out
```

`init` is construction and `destroy` is destruction — a class that writes
`destroy` owns something, and then it must also say how it is copied, by an
`init` taking one of itself. Every method is virtual and single inheritance is
written `class Square(Shape):`. An operator is a method: `operator+`,
`operator==`, `operator[]`, `operator=`.

A construction is written as a call on the type's own name, and it is a value
and not a pointer — `new` is what gives a pointer back:

```haard
let origin = Point()               # a value, built here
let pair = Pair<i32, i32>(1, 2)    # a generic, written with its arguments
let text = String("from a char*")  # nothing converts on its own; this asks
let n = i32(3.5)                   # a builtin, and the same as '3.5 as i32'
```

### Enums, and the switch that reads them

An enum is a **tagged union**: a value is exactly one of its variants, and a
variant may carry something.

```haard
enum Action:
    Idle
    Click : (i32, i32)
    Code = 7
    Quit

def describe : i32
    @a : Action

    switch a:
        case Click(x, y):
            return x + y

        # cases group by writing one with no block -- there is no fall
        # through and no 'break'
        case Idle:
        case Quit:
            return 0

        default:
            return -1
```

A variant that carries something is a constructor: `Action.Click(10, 20)`, or
bare `Click(10, 20)` where no other name wins. A switch over an enum must
cover it or write `default`. A switch over an integer or a char is a C++
switch; over a `String`, a float or any class with `==` it becomes a chain of
`if`s, so `switch` is worth writing over anything.

### Symbols

A symbol is an interned name. Every `:foo` in the program is a pointer to the
same entry of one immutable table the emitter builds, so comparing two symbols
is comparing two addresses, and the entry gives the text back.

```haard
let a = :ok
let b = :ok

if a == b:                 # one pointer comparison
    println(a)
```

It is a type of its own, `symbol`, so a field or a parameter may be one, and
`:foo == "foo"` is a mistake about types.

### Strings, containers and generics

```haard
import std.io
import std.string

def main : i32
    let xs : i32[] = [3, 1, 2]     # Array<i32>, written form 'T[]'
    xs.add(4)
    xs[0] = 5

    let ys : List<i32> = [1, 2, 3] # a doubly linked list, in Haard

    let ages : Hash<symbol, i32>   # open addressed, in Haard
    ages[:ada] = 36
    ages[:grace] = 45

    for who in ages:               # a Hash walks its keys
        println("${who as char*} is ${ages[who]}")

    let total = 0

    for x in xs:
        total = total + x

    let line : String = "total: ${total}, and ${ys[2]}"

    println(line)

    return 0
```

`${...}` inside a string is a template string, taken apart before the emitter
into a `String` and a few `append` calls. `[1, 2, 3]` is a fixed array plus one
constructor call, and which constructor is chosen by the type written on the
left. Generics are monomorphised: `Hash<symbol, i32>` becomes a class of its
own, in the module that declared the generic.

### The standard library

`std/` is seven files, all of them Haard:

| module | what is in it |
|---|---|
| `std.io` | `print` and `println`, overloaded on `char*`, `String&`, `char`, `i32`, `i64`, `u32`, `f64`, `bool` and `symbol` |
| `std.string` | `String`, which owns its bytes and is what a `${}` builds |
| `std.array` | `Array<T>`, the class `T[]` is written form for |
| `std.list` | `List<T>`, a doubly linked list |
| `std.hash` | `Hash<K, V>`, open addressed, hashed by the `hash_of` overload set |
| `std.file` | `File`, `console()`, `open_read`, `open_write` — the same names as `std.io`, on a file |
| `std.low_io` | eight functions whose bodies the compiler writes, one character at a time |

Only that last file is special, and it is meant to be deleted. Nothing in
Haard can reach a C library yet, so the emitter fills in eight bodies — by
name, and only inside `std.low_io` — and everything anyone actually calls is
built on them, in the language, where it can be read and changed without
touching the compiler. When a real foreign interface is decided, those eight
become ordinary declarations of it and nothing above them changes.

### Pointers

Haard has pointers, `new`, `delete` and `delete[]`, and no garbage collector.

```haard
def strlen : i32
    @s : char*

    let counter = 0

    while s[counter] != '\0':
        counter = counter + 1

    return counter

def main : i32
    let v = new i32[10]

    for i in 0 ... 10:
        v[i] = i * i

    let p = &v[2]

    *p = 0

    delete[] v

    return 0
```

### Operators and precedence

Haard's precedence is **not C's and not Python's**, and the difference is on
purpose. The parser is a cascade of one function per level, so the table below
is the parser read from top to bottom: **loosest first**, everything under a
row binds tighter than that row.

| Level | Operators | Associativity |
|---|---|---|
| assignment | `=` `+=` `-=` `*=` `/=` `//=` `%=` `&=` `\|=` `^=` `~=` `<<=` `>>=` `>>>=` | right |
| cast | `as` | see below |
| logical or | `or` `\|\|` | left |
| logical and | `and` `&&` | left |
| equality | `==` `!=` | left |
| relational | `<` `>` `<=` `>=` `in` `not in` | left |
| range | `..` `...` | left |
| additive | `+` `-` | left |
| multiplicative | `*` `/` `//` `%` | left |
| power | `**` | left |
| bitwise or | `\|` | left |
| bitwise xor | `^` | left |
| bitwise and | `&` | left |
| shift | `<<` `>>` `>>>` | left |
| unary prefix | `!` `not` `~` `-` `+` `++` `--` `&` `*` `**` `new` `delete` `delete[]` `sizeof` | right |
| postfix | `.` `->` `[]` `()` `++` `--` | left |
| primary | `::` a name, a literal, `(...)`, `[...]`, `{...}`, `\|...\|`, `this`, `super`, `${}` | |

`or`/`||` and `and`/`&&` are one operator with two spellings, and so are
`not`/`!`. Which one was written is kept, and the pretty printer writes it back.

The `&` and `*` that stand between two operands are the bitwise and and the
multiplication; the ones that open an operand are the address-of and the
dereference. What tells them apart is only where they are read. `**` is a
power between two operands and a **double dereference** in front of one, so
`**p` is `*(*p)`.

#### Where it differs from C++

Five levels — `**`, `|`, `^`, `&` and the shifts — sit **between the
multiplication and the unary operators**, so all five bind **tighter than `*`
and `/`, and much tighter than `+` and `-`**. The four C also has are all
looser than arithmetic there, so this is the reverse of C:

```haard
4 + 3 & 1        # Haard: 4 + (3 & 1)     C: (4 + 3) & 1
1 << 2 + 3       # Haard: (1 << 2) + 3    C: 1 << (2 + 3)
2 * 3 | 4        # Haard: 2 * (3 | 4)     C: (2 * 3) | 4
```

The C order is the one that makes `if (a & MASK == 0)` a famous bug. Haard's
answer is that a bitwise operator is arithmetic on the representation, so it
binds like arithmetic that is tighter still, and the parentheses C needs are
not needed here.

`as` is the other difference: it is looser than every operator except
assignment, where a C++ cast binds as tight as a unary operator. Its left
operand is the whole expression to its left, and **nothing may follow it** —
there is no level between `as` and assignment for an operator to be read at.

```haard
let b = 1.0 + a as i32       # (1.0 + a) as i32
let b = a as i32 + 1         # error: nothing may follow a statement on its line
```

There is no ternary `?:` and no comma operator.

#### Where it differs from Python

`**` is **left** associative and looser than the bitwise operators, where
Python's is right associative and the tightest binary operator it has. And a
unary minus binds **tighter** than `**`, not looser:

```haard
2 ** 3 ** 2      # Haard: (2 ** 3) ** 2 = 64      Python: 2 ** (3 ** 2) = 512
-2 ** 2          # Haard: (-2) ** 2 = 4           Python: -(2 ** 2) = -4
```

`not` is a unary operator sitting with `!` and `~`, not a low precedence word,
so it takes the operand next to it and not the comparison around it:

```haard
not a < b        # Haard: (not a) < b     Python: not (a < b)
```

Comparisons do not chain. `a < b < c` is `(a < b) < c`, and since the left half
is a `bool` the compiler says so rather than reading it as `a < b and b < c`.

`in` and `not in` are at the relational level, which is where Python has them
too.

#### Two rules that are not precedence but decide the same thing

A `<` **glued** to a name opens a generic argument list; **spaced**, it is a
comparison. `f<i32>()` calls a generic and `a < b` compares — spacing is what
disambiguates, and `a<b` glued to something that cannot take type arguments is
an error that says as much.

An operator must be on the **same line** as its left operand, unless a `(`,
`[` or `{` is open — inside brackets one expression may span as many lines as
it likes.

#### What is parsed but not yet typed

`**` between two operands, and `in`/`not in` outside a `for`, are read and
printed but have no type and no emission yet. The compiler says so by name
rather than emitting something that means the wrong thing.

## Where the compiler is

**Works, and every line of it is proven by a program that runs:**

| | |
|---|---|
| classes, fields, methods, `this`, `.` and `->` | single inheritance, overriding, covariant returns |
| `init` and `destroy`, copy by an `init` | overloading, chosen by signature |
| generics, monomorphised | operators as methods, including `=` and `[]` |
| type inference, `let` optional | enums as tagged unions, `switch` as a pattern match |
| `for x in` over containers, arrays and ranges | template strings, `${}` |
| symbols, interned in a table | `Array<T>`, `List<T>`, `Hash<K, V>`, `String`, `File`, `print`/`println` |
| imports, aliases, star imports, cycles | two versions of one library in one program |
| `haard.pkg`, followed transitively | rustc-shaped diagnostics with carets |
| pointers, `new`, `delete`, `new T[n]` | modules compiled to one C++17 file |

**Not there yet**, and the compiler says so by name rather than emitting
something that means the wrong thing:

| | |
|---|---|
| closures and lambdas | tuples as values |
| `{key: value}` as a literal with a type | `T(args)` — calling a constructor by hand |
| a range as a value (`let r = 0 .. 10`) | `main(argc, argv)` |
| versions, a registry, a lock file | move semantics, `const` |
| threads, exceptions | a filesystem beyond open/read/write/close |

## Tests

The test suite has no external dependencies and no framework: it needs `g++`,
`bash`, `diff` and `timeout`, all of which you already have if you can build
the compiler. From the `build` folder:

```
make check
```

Seventeen suites and about 660 cases, in roughly a minute. It exits 0 when
everything passes and 1 otherwise, so it works in a git hook or in CI. Each
suite can also be run on its own:

```
./tests/scanner/run.sh
./tests/emitter/run.sh
./tests/programs/run.sh
```

Most suites are golden based: a small driver prints what one phase produced —
tokens, the tree, the symbol table, the types, the resolutions, the emitted
C++ — and the output is compared against a committed file. Diagnostics are part
of that comparison, so the wording of every error message is covered.

Two suites are different and are the ones that matter most:

* **`tests/programs`** runs the flow a *user* runs: the real `hdc` binary,
  through each case's own `Makefile`, into C++, into a binary, which is then
  executed and **checks its own work**. A PPM writer, a drawing library, a
  binary tree over an enum, a hash written in Haard, and a program built from
  manifests all live there.
* **`tests/emitter`** compiles the C++ it generated with `g++` and runs it, and
  the golden ends with the binary's exit status. A transpiler is right when its
  output runs and gives the right answer, and nothing else proves that.

The scanner suite also carries **invariants** that must hold for any input:
each token's lexeme is exactly `source[offset, length)`, tokens are ordered and
never overlap, only whitespace or a comment sits between two of them, and the
indentation counter matches the line the token starts on. The per-case
`timeout` is a test in its own right — a scanner that stops making progress
shows up as `TRAVOU` instead of hanging the suite.

To add a case, drop it in the suite's `cases/`, run `./run.sh -u` to record its
output, **read the resulting `git diff` of `expected/`** to confirm the output
is actually correct, and commit both. That last step is the one that matters:
`-u` blindly accepts whatever the compiler produced, bugs included. And every
case here is expected to be proven by **breaking the code it covers** and
watching it fail.

Cases listed in `tests/scanner/known_failures.txt` document bugs that are known
and not fixed. They report as `XFAIL` and do not fail the suite; if one starts
passing, the suite reports `XPASS` and fails, so the list cannot go stale.

## Code formatting

The project ships a `.clang-format` describing the style used across `src/`.
With `clang-format` installed (`sudo apt install clang-format`), from the
`build` folder:

```
make format         # rewrite the sources in the project style
make format-check   # check only, fails if anything is off
```

If `clang-format` is missing, the target still exists and fails with a message
telling you how to install it.

## Documentation

`docs/design/` holds one record per decision, numbered in the order they were
made — what was decided, what was rejected, and why. `docs/STATE.md` is the
state of the code: what works, what does not, and what to do next.

## The name

The name is a play on the word 'hard', because one of the goals of the language
is systems development (hard -> hardware -> operating system). Also, I wanted
to have some fun saying that 'haard is not hard :)'. It also seems that Haard
means fireplace or something like that in Dutch (that's why the logo is a
flame). To finish, my name is Hadley and I work with hardware (FPGAs)... Oh,
well. I am not good with names.

## Why

There are some reasons for writing this:

1. I've always wanted to learn how compilers work in detail, and I believe this
   project can help me with that.
2. I like to work with homemade softcore processors in FPGA, and with this
   project I aim to have a compiler I can modify at will.
3. LLVM is a great project, but using it would be against the objective of
   item 1.
4. I like to program at a low level but also like high-level stuff. I missed
   features in C and C++ that were present in other languages:
    * a simple import instead of include + ifdef
    * not worrying about recursive file inclusions
    * type inference without being verbose (no `auto` keyword and similar
      stuff)
    * closures
    * a nice standard library without having to resort to boost with verbose
      syntax
