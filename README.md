*For God so loved the world, that he gave his only begotten Son, that*
*whosoever believeth in him should not perish, but have everlasting life.*

*For God sent not his Son into the world to condemn the world; but that*
*the world through him might be saved.*

*John 3:16-17*

# The Haard Programming Language

Haard is a programming language with syntax inspired 
mainly from Python, but the semantics are more similar to C++. 

This repository contains the source code for the first compiler
of the language, which is being implemented in C++. It is still in
development and can't produce a 'hello, world' yet.

## Install from source

1. Clone the repository
2. Enter the directory
3. Create a folder named build and enter it
4. Run cmake
5. Run makefile
6. There will be a binary named hdc in the build folder

```
git clone https://github.com/hadley-siqueira/haard.git
cd haard
mkdir build
cd build
cmake ..
make
```

Examples of code can be found at the samples folder. Note, however, that
they are just example of code. Many of them are not running yet.

Note that `src/haard/parser/parser.cpp` is a work in progress and does not
compile yet, so `make` currently fails on that one file. Every other part of
the compiler builds, and the scanner is complete enough to be tested on its own.

## Tests

The parser is not functional yet, so the test suite covers the scanner only. It
has no external dependencies: it needs `g++`, `bash`, `diff` and `timeout`, all
of which you already have if you can build the compiler. There is no test
framework and no extra binary in the repository — the two small drivers are
compiled on the fly into a temporary directory.

```
./tests/scanner/run.sh
```

Or, from inside the `build` folder, `make check`. The suite exits with 0 when
everything passes and 1 otherwise, so it works in a git hook or in CI.

Each file in `tests/scanner/cases/*.hd` is fed through two checks.

**Golden tests.** `dump_tokens.cpp` prints the token stream in a stable text
format — one line per token with its kind, offset, length, whitespace flag,
indentation counter and lexeme — and the output is compared against the
committed `tests/scanner/expected/<case>.txt`. Scanner error messages are part
of that comparison, so the wording of the diagnostics is covered too.

**Invariants.** `check_invariants.cpp` verifies properties that must hold for
*any* input, so they also catch regressions in files nobody wrote an expected
output for:

* *round trip* — each token's lexeme is exactly `source[offset, length)`, tokens
  are ordered and never overlap, and whatever sits between two tokens is only
  whitespace or a comment. No byte of code is lost or duplicated.
* *whitespace sensitivity* — the indentation counter matches the spaces at the
  start of the line where the token begins; tokens on the same line share the
  line flag; tokens on different lines have it flipped.

The per-case `timeout` is a test in its own right: a scanner that stops making
progress shows up as `TRAVOU` instead of hanging the suite.

To add a case, drop a `.hd` file in `cases/`, run `./tests/scanner/run.sh -u` to
record its output, **read the resulting `git diff` of `expected/`** to confirm
the output is actually correct, and commit both files. That last step is the one
that matters: `-u` blindly accepts whatever the scanner produces, bugs included.

Cases listed in `tests/scanner/known_failures.txt` document bugs that are known
and not fixed yet. They report as `XFAIL` and do not fail the suite; if one of
them starts passing, the suite reports `XPASS` and fails, so the list cannot go
stale. See `tests/scanner/README.md` for more.

## Code formatting

The project ships a `.clang-format` describing the style used across `src/`.
With `clang-format` installed (`sudo apt install clang-format`):

```
make format         # rewrite the sources in the project style
make format-check   # check only, fails if anything is off
```

Both are run from the `build` folder. If `clang-format` is missing, the target
still exists and fails with a message telling you how to install it.

## The language

The programming language is named Haard. The name is a play of the 
word 'hard' because one of the goals of the language is to be able to do
system development (hard -> hardware -> operating system). Also, I wanted 
to try to do some fun by saying that 'haard is not hard :)'.
It also seems that Haard means fireplace or something like that in Dutch 
(that's why the logo is a flame). To finish, my name is Hadley and I work with
hardware (FPGAs)... Oh, well. I am not good with names. 

Haard is a compiled language and it will try to provide the following:

* Type inference
* Templates
* Classes
* Closures and/or lambdas
* A nice standard library
* Low-level access (for system development such as operating systems)
* Other stuff that I don't remeber right now

Below are some examples of the language syntax. The syntax is inpired mostly on 
Python, but Ruby, VHDL, C++, Java, Rust and others are used also as inspiration. 
Although the syntax is Python-like, the semantics are more similar to the ones of
C and C++.

You may wonder why I am creating this language. There are some reasons:

1. I've always wanted to learn how compilers work on detail and I believe this project
  can help me on this.
2. I like to work with homemade softcore processors in FPGA and with this project I aim to have
  a compiler that I can modify to my will
3. LLVM is a great project but using it would be against the objectives of item 1 of this list
4. I like to program in low-level but also like high-level stuff. I missed features in C in C++ that were
   present in other languages but not in C and C++. E.g.: 
    * use a simple import/use instead of include+ifdef. 
    * do not worry about recursive file inclusions
    * type inference without being verbose (no need to auto keyword and similar stuff)
    * closures
    * a nice standard library without having to resort to boost with verbose syntax

There are other reasons but I guess I don't need to explain all of them right now.

### A simple 'Hello, world'

```python
import std.io

def main : void
    println('hello, world!')
```

### A simple sum function

A function to add two numbers. Note that parameters are declared one per line. The syntax here
was inspired by VHDL where it is common to declare one input/output per line. The @ precedes the
parameter's name and then there is the type of the parameter. Comments uses # and function calls
syntax is similar to other languages

```python
# a function that returns an int
import std.io

def sum : int 
    @a : int
    @b : int

    return a + b


def main : int
    a = 2
    b = 5

    println('The sum is ' + sum(a, b))
    return 0
```

#### What about those @?

I care about presentation. Did you already came by a C++/Java/Alike code such as:

```C
GArray*
g_array_sized_new (gboolean zero_terminated,
                   gboolean clear,
                   guint    elt_size,
                   guint    reserved_size)
{
```

The example is from glib. Do you see how there is one parameter per line? Well, its awkward
to me to see a '(' on one line and then '{' on next line. So I decided to use:

```python
def g_array_sized_new : GArray*
    @zero_terminated : gboolean
    @clear           : gboolean
    @elt_size        : guint
    @reserved_size   : guint
```

### Control flow

A sample program that shows some control flow

```python
import std.io

def main : void
    i = 0

    while i < 10:
        println(i)
        ++i

    for i = 0; i < 10; ++i:
        println(i)

    for i in 0 .. 10:
        println(i)

    for i in range(0, 10):
        if i > 5:
            println('i is greater than 5')
        else:
            if i == 0:
                println('i = 0')
            elif i == 1:
                println('i = 1')
            else:
                println('i <= 5')
```

### Pointers

As I said, Haard has semantics similar to C and C++ and it also has pointers.

```python
import std.io

def main : int
    @args : String[]

    println('Running program ' + args[0] + '...\n')
    size = int(input('Enter the size: '))

    v = new int[size]

    for i in 0 .. size:
        v[i] = int(input('Enter a number: '))

    print('v = {')

    for i in 0 .. size - 1:
        println('' + v[i] + ', ')

    # its just a demo. Don't blame me for invalid memory access
    println('' + v[size - 1] + '}')

    delete v
    *(v + 2) = *v # uh oh, maybe a segfault here
```

Function with pointer parameters:

```python
# a simple struct
struct Pixel:
    r : u8 # u8 is equivalent to uint8_t
    g : u8
    b : u8

def negative : void
    @pixel : Pixel*

    pixel->r = 255 - pixel->r 
    pixel->g = 255 - pixel->g
    pixel->b = 255 - pixel->b

def strlen : uint
    @s : char*

    counter = 0

    while *s != '\0':
        ++s
        ++counter

    return counter

# template here
def swap<T> : void
    @p1 : T*
    @p2 : T*

    tmp = *p1
    *p1 = *p2
    *p2 = tmp

def create_matrix : int**
    @n : int
    @m : int

    v = new int*[n]

    for i in 0 .. n:
        v[i] = new int[m]

    return v

def main : void
    p = 'hello'
    strlen(p)

    a = 2
    b = 3
    swap<int>(&a, &b)

    matrix = create_matrix(10, 20)

    pixel = Pixel()

    pixel.r = 0
    pixel.g = 0
    pixel.b = 0

    negative(&pixel)
```

### Classes

A simple Player class

```python
import game.field.position

class Player:
    life : int # class variables
    x    : int
    y    : int

    # constructor
    def init : void
        @life : int
        @x    : int
        @y    : int

        this.life = life
        this.x = x
        this.y = y

    def is_alive : bool
        return life > 0 
