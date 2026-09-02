# StringTable tests

No frameworks. Only needs `g++`, `bash` and `timeout`.

```
./run.sh          # run everything, one line per check
```

Or, with the other suites, from the build directory: `make check`.

**There is no `expected/` directory here**, and that is the point. The other
suites compare against a golden, which records whatever the code produced and
is reviewed by reading the diff. These checks carry their own answers, because
what they pin is a published specification — FNV-1a's own test vectors — and a
golden would happily record a wrong hash as the right one.

## What the checks are pinning

**That the hash is FNV-1a and not something near it.** The empty string must
hash to the offset basis untouched, which is the one case that proves the basis
alone. `"a"` and `"foobar"` are FNV-1a's published vectors. And `"a"` must not
hash to `0x050c5d7e`, which is what **FNV-1** gives for it: the two differ only
in whether the xor comes before or after the multiply, so a transcription slip
in the Haard rewrite would produce a working hash that silently disagrees with
every blob written by the bootstrap.

**That the byte comparison in `find` is load-bearing**, with two real
collisions. `costarring`/`liquid` and `declinate`/`macallums` are documented
FNV-1a 32-bit collisions, so `two_names_of_one_hash_stay_two_names` is not a
contrived case: without the confirm, `liquid` comes back as the index of
`costarring`, and every use of one name resolves to the other's declaration.

**That the cross-module probe of record 0013 works the way the record
describes.** `a_name_is_found_by_a_hash_computed_elsewhere` has two tables and
asks the second one about a name using a hash the first one computed, because
that is what a lookup across an import does: the importer's own index means
nothing in the dependency's table, so `find` takes a hash and bytes, never an
index.

## The sabotages

Every check above fires. The counts are the number of check lines that fail:

| sabotage | fails |
|---|---|
| FNV-1 instead of FNV-1a — multiply, then xor | 4 |
| a wrong offset basis (`0` instead of `0x811c9dc5`) | 4 |
| `find` trusts the hash and skips the byte comparison | 5 |
| `intern` always appends instead of deduplicating | 3 |

**One sabotage does not fire, and it is worth knowing why.** Accumulating in a
`u64` and truncating only at the end gives *the same answer*: multiplication
modulo 2³² agrees with the low 32 bits of the wider product, and the xor only
touches the low byte. So the width of the accumulator genuinely does not
matter — only the width of the value that gets **stored** does, and in C++ the
return type settles that. A Haard implementation that accumulated in 64 bits
and stored 64 would differ, and nothing here would catch it, because nothing
here can construct that.
