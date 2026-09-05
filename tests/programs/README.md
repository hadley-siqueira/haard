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
