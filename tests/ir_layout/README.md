# IR layout tests

No frameworks. Only needs `g++`, `bash`, `diff` and `timeout`.

```
./run.sh          # run everything
./run.sh -u       # rewrite the expected/ files with the current output
```

Or, with the other suites, from the build directory: `make check`.

Stage 1 of the IR (record 0077): the program's types translated into the IR's
(`IrTypeTranslator`), one table for the whole program (`IrTypeTable`), laid
out for one target (`LayoutTable`), and every class's vtable (`IrClassTable`). Every directory in `cases/` is a project,
with an optional `target` file naming `x86_64-linux` (the default),
`arm64-linux` or `pret`. The golden is every aggregate with its size,
alignment, data size and each field's offset:

```
struct FromBuilt: size 8, align 4, data 6
    base Built at 0
    c : b8 at 5
```

## g++ checks every layout

Hadley chose the Itanium C++ ABI's rules for inheritance on 2026-10-20 so that
g++ could check every layout, classes with a base included. So on an x86-64
machine, for an `x86_64-linux` case, the suite also:

1. asks `layout --probe` for the C++ the emitter writes for the program, plus
   one typedef per aggregate and a function, run before `main`, that prints
   what `sizeof`, `alignof` and `offsetof` say about each;
2. builds and runs it;
3. compares what it printed with `layout --facts`, the same facts from the IR.

A difference fails the case as `g++ lays it out otherwise`, and a passing case
says `(g++ agrees)`. The golden says what the layout **is**; the probe says it
is what the C++ path has been doing all along. A case whose golden is an error
has nothing to probe, and the `arm64-linux` and `pret` cases have only their
goldens.

An enum's tag is the smallest unsigned width that holds its largest tag, in the
IR and, since the same day, in the emitted C++ (`enum class X : uint8_t`, and
the tagged union's `tag`), so the probe covers enums too.

## What the Itanium rules are, as the emitted C++ makes them

Found by asking g++ before writing the code, and pinned by
`a_base_lends_its_tail_padding`:

| The Haard struct | The C++ | POD for the purpose of layout |
|---|---|---|
| fields only, methods | an aggregate | yes |
| an `init` | a constructor | no |
| a `destroy` | a destructor | no |
| a field given a value | a member initialiser | no |
| a reference field | a reference member | no |
| a base | a derived struct | no |
| a field whose type is not POD | | yes, still |

A base that is not POD lends its **tail padding**: `FromBuilt`'s `c` is at 5,
inside `Built`'s 8 bytes; `FromPlain`'s is at 8. A class is never POD (it has a
vtable), so a class derived from a class starts in its base's padding:
`Derived`'s `b` at 9 in `a_class_puts_its_vptr_first`.

An **empty** struct takes 1 byte alone and none as a base, unless that would put
two empty subobjects of one type at one address: `Twice` derives from `Nothing`
and holds a `Nothing` first, which moves to 1.

## The class table

After the layouts, the golden lists every class's vtable (`IrClassTable`,
Hadley 2026-10-20): slot 0 the destructor, then the base's slots, an override
taking the slot of what it overrides, then the new methods; `init` takes none,
and a struct base brings none. The entry before slot 0 is the place of the
class's descriptor (introspection), which no method moves for.

```
vtable of Square:
    before 0: the descriptor
    0: destroy(), by Square
    1: area(), by Square
    2: perimeter(), by Shape
```

g++ does not check these: the emitted C++'s Itanium vtable is a different
one on purpose (two destructor entries, a slot for `init`, RTTI), so only the
goldens and the sabotage below pin it.

## Sabotage

Each rule was broken on purpose, one at a time, and the suite run. A restored
file has to be touched afterwards, or the shared object cache keeps the
sabotaged object: its date is older than the object built from the sabotage.

| Broken | Fails |
|---|---|
| a struct base lends no tail padding | `a_base_lends_its_tail_padding` |
| a class base lends no tail padding | `a_class_puts_its_vptr_first` |
| an `init`, a `destroy`, a field value: each in turn not breaking POD | `a_base_lends_its_tail_padding` |
| an empty base takes its byte | `an_empty_struct_takes_one_byte` |
| no check for two empty structs at one address | `an_empty_struct_takes_one_byte` |
| no vtable pointer | three cases |
| a union's fields not at 0 | `a_union_overlays_its_fields`, the enum case |
| the tag's width, in the IR | `an_enum_is_a_tag_and_a_union` |
| the tag's width, in the emitter only | the same, as `g++ lays it out otherwise` |
| `isize` 4 bytes wide | `scalars_take_their_natural_size` |
| a tuple interned by its Haard type | `a_tuple_is_a_struct_per_shape` |
| a declaration interned only in the first module | `one_type_seen_from_two_modules` |
| a function value's two fields swapped | `a_function_value_is_two_pointers` |
| `pret` little endian | `padding_on_pret` |
| no check for a type holding itself | `a_struct_that_holds_itself_has_no_size` |
| an override never takes its base's slot | three cases |
| an override matched by name only | `an_overload_takes_a_slot_of_its_own`, `an_override_crosses_a_module` |
| the return compared too, against covariance | `a_vtable_starts_with_the_destructor` |
| `init` takes a slot | `a_vtable_starts_with_the_destructor` |
| `destroy` an ordinary slot, not slot 0 | `a_vtable_starts_with_the_destructor` |
| a struct base brings its slots | `a_struct_base_brings_no_slots` |
| a named parameter type compared without its module | `an_override_crosses_a_module` |
