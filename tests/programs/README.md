# Whole-program tests

Every other suite takes the compiler apart. This one does not: each case is a
**project a person could have written**, and it is run the way a person runs
it.

    hdc --roots roots.tbl myapp/main.hd --emit-cpp  >  main.cpp
    g++ -std=c++17 -o app main.cpp
    ./app

Three things make it different from `tests/emitter`, which also builds and runs
what it emitted:

1. **The real `hdc` binary**, and not a driver built for the occasion. What
   this suite proves is that the program a user compiles works.
2. **The case's own `Makefile`** does the building. So there is one description
   of how a Haard program becomes a binary, it lives where a reader would look
   for it, and it cannot rot: if it stops being true, this suite stops passing.
3. **The programs check their own work.** They write files, read them back and
   print what held, so a golden of a few lines pins a great deal.

## What a golden holds

What the program printed, the files it left behind by name and size, and its
exit status.

```
white 8x6 read back: ok
gradient 64x48 survived the round trip: ok
inverted is 255 minus the gradient: ok
--- files:
gradient.ppm, 34125 bytes
inverted.ppm, 34557 bytes
white.ppm, 587 bytes
--- exit: 0
```

The size line is what catches an image that changed without anything printing
differently.

## Running one by hand

Each case is a working project. From its directory:

    make          # build ./app
    make run      # build it and run it
    make cpp      # stop after the C++, to read it
    make check    # just check the program, emitting nothing
    make clean

The Makefile finds `hdc` at `../../../../build/hdc`; pass `HDC=` for anywhere
else. The suite copies each case to a scratch directory before building, so
running it never writes into the repository — by hand, in the case's own
directory, it does, and `make clean` takes it back.

## The cases

- **`an_image_written_and_read_back`** — a PPM reader and writer in Haard, over
  a `std` that is itself Haard on top of record 0030's eight native functions.
  Three roots: `std`, `ppm` and the user's `myapp`, which sees the other two
  and nothing they import privately. It paints an image white, writes a
  gradient, reads both back, inverts one and reads that back, and compares
  every channel. The inversion matching is what proves the **reader**: it can
  only hold if every value came back exactly.

  It found two compiler bugs the day it was written, both a type index used in
  the wrong module's table.

- **`shapes_and_text_are_drawn`** — a drawing library in Haard on top of that
  one, in a fourth root: lines (Bresenham), rectangles filled and hollow,
  circles (midpoint), quadratic and cubic Béziers, and fixed-size text from a
  bitmap font. All of it integer arithmetic; a Bézier clears its fractions by
  multiplying the polynomial through by the step count, so there is no float
  anywhere and no rounding drift.

  It checks its work twice over, and the two halves catch different things.
  **The golden holds the picture**, drawn in `#` and `.`, so a shape that
  moves by one pixel shows up in a diff nobody had to anticipate — the text
  preview literally spells HAARD. The **assertions** under it are what reading
  a picture cannot tell you: that a circle's four cardinal points are lit,
  that an outline is empty inside, that a curve meets its ends and misses its
  handle, that every glyph is exactly 5 by 7.

  It found the bitwise operators typing to nothing.

- **`shapes_and_text_by_template`** — `shapes_and_text_are_drawn` again, with
  record 0032's template strings. **The drawing is identical on purpose**: the
  picture in the two goldens is byte for byte the same, so a diff between the
  cases is exactly what the sugar changed, and nothing else.

  What it changed is that a message carries the numbers it is about —
  `"circle at ${48},${8} radius ${6} has its four cardinal points"`, and a
  count that used to be three statements and a helper is one `writeln`. `File`
  grew `write` and `writeln` for a `String&` so that a `${}` at a call site
  does not have to end in `.text()`, and `report` takes a **`String` by
  value**: a message with no `${}` in it is a `char*` (record 0022) and
  reaches one through record 0023's coercion, which is one step, while a
  `String&` would need two and nothing has decided that coercions chain.

  No file of the program imports `String`. The table's `prelude` block does it
  (record 0033), which is what makes a template string mean anything here.

- **`lists_are_built_from_literals`** — record 0037, and the two ways there
  are to build a `List<T>`, since there is no literal for one: from a bracket
  literal, where the compiler picks `init(Array<T>&)`, and from a braced one,
  where it picks `init(T*, i32)`. It writes through `operator[]` (which walks,
  from the head), pushes both ends, and checks that a copy holds its own nodes.

  `List<T>` holds a `Node<T>*`, which is the shape that forced the body of an
  **unbound generic** to stop being checked: two occurrences of `Node<T>` in
  two generics are two types.

- **`arrays_are_written_as_sugar`** — `Array<T>` written as `i32[]`, in a
  program that imports nothing: Array, String, File and `console()` all arrive
  through the table's `prelude` block. It grows an array past its capacity
  three times, reads it through record 0034's `operator[]`, writes through the
  `T&` that gives back, checks that a copy holds its own buffer (record 0031),
  builds an `i32[][]` — which is where a generic naming itself would bite —
  and an `Array<String>`, so every element copies with the element's own
  `copy`. Its messages carry their numbers, which is record 0032.

- **`the_prelude_is_written_in_the_table`** — record 0033, and the point of it
  is what is **not** at the top of `myapp/main.hd`. It writes one import, its
  own sibling, and names nothing of the library — then builds a `String`, opens
  the console, writes a file and reads it back. `String`, `File`, `console`,
  `open_write` and `open_read` all arrive from the `prelude` block of
  `roots.tbl`.

  `myapp/report.hd` next to it **does** write `import std.string`, which the
  table gives it anyway. That is the case record 0033's second guard exists
  for: a second dependency on one module is a second copy of every candidate,
  and an ordinary call would be reported as matching more than one thing
  equally well.

- **`strings_are_built_and_joined`** — `String` itself, written in Haard: it
  owns its bytes (record 0023) and writes the `copy` record 0031 requires, so
  all four ways of giving a value to something work and no two Strings hold
  one buffer. The case checks that a copy really is its own by poking one and
  reading the others.

  It is also where `String` turned out to be **its own builder**: `append` is
  overloaded for every kind, so the template string that record 0025's
  Ast → Ast pass will write becomes calls to those and there is no
  `StringBuilder`. `let hello : String = "hello"` is record 0023's coercion
  running for the first time.

  It found the coercion list having no entry for a value where a reference was
  expected, and a literal only learning its type from the left.
