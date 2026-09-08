# 0041 — A symbol is an interned name, and a type of its own

Status: **decided and implemented**, 2026-09-08. Hadley gave the design whole:

> `:símbolo` deve ser um ponteiro pra char gerado no emissor que aponta sempre
> para o mesmo local. É a mesma ideia de ruby: um simbolo sempre aponta para o
> mesmo local internamente. Deve ser imutável. No emissor de C++ deve ser
> gerada uma tabela fixa e imutável e os ponteiros representando os symbols
> apontam para a entrada respectiva dessa tabela. Assim, todo symbol `:foo` por
> exemplo vai apontar para a entrada `foo` nessa tabela. Assim comparação de
> symbols fica tão rápido quanto fazer um `u32 == u32` e a tabela permite
> recuperar a representação em `char*` do symbol.

| | |
|---|---|
| One table for the program, one entry per distinct name, built by the **emitter** | **decided**, Hadley |
| Every `:foo` is the **pointer to its entry**, so two of them compare as addresses | **decided**, Hadley |
| The table is **immutable**, and so is what it points at | **decided**, Hadley |
| The entry holds the text, so a symbol can be read back | **decided**, Hadley |
| A symbol is a **type of its own** and not a `char*` | **decided while writing it**, see below |
| `symbol` is a keyword, so a parameter and a field may be one | **decided while writing it** |

## What it lowers to

```haard
let a = :status
let b = :'status'
let c = :other

def same : bool
    @x : symbol
    @y : symbol
    return x == y
```

```cpp
static const char* const __symbols[] = {
    "status",
    "other",
};

bool h0_1_same(const char* x, const char* y) {
    return x == y;
}

const char* a = __symbols[0];
const char* b = __symbols[0];
const char* c = __symbols[1];
```

The table is spliced **above** the constants and the globals, since a global
may name a symbol and nothing may name one before it exists. It is written
from a map kept for the whole compilation, so `:foo` in two modules is one
entry and one address — which is the whole of what makes the comparison a
comparison of pointers.

`:foo` and `:'foo'` are **one** symbol: the quotes are how a name with a space
is written and are not part of the name.

## The one thing the design did not say

Whether `:foo` and `"foo"` are the same type to the checker.

They are **not**, and the reason is the model Hadley named: in Ruby a Symbol is
not a String, and `:foo == "foo"` is false rather than a question about bytes.
Making a symbol a plain `char*` would have made that comparison *compile* and
answer false — a footgun the language has no reason to hand out — and would
have made `:foo` nothing but an interned string literal.

So a symbol is **its own builtin**, `BUILTIN_SYMBOL`, represented as a
`const char*`. Everything falls out of record 0018 having no conversion:

- `:foo == "foo"` is *cannot apply this to symbol and char\**
- a symbol reaches a `char*` parameter only by being written across, `s as
  char*`, which is the "recuperar a representação" half of the design
- `:a + :b` is *a symbol is a name and has no arithmetic*, refused in the typer
  rather than emitted as C++ pointer arithmetic

Reverting the decision is one line in `ExpressionTyper::work`, which is why it
was taken rather than asked about.

A builtin and not a named type, because a builtin holds the **same index in
every module** (record 0016 seeds them at fixed positions), so a symbol crosses
a module boundary untranslated the way a `bool` does.

## `symbol` is a keyword

A type nobody can write is a type nobody can use: without the word, no
parameter, no field, no `let s : symbol`. So `symbol` joins `bool`, `void` and
`char` in the scanner's keyword table, and `symbols`, `_symbol` and `Symbol`
stay identifiers.

## What it costs

**One more builtin**, which every module seeds — the type_table goldens all
went up by one interned type, and that is the whole of the cost.

**Nothing else in the compiler knows what a symbol is.** The typer gives it a
type, the emitter gives it an entry, and every other phase reads a builtin it
has no case for.

## What is not decided here

**A symbol in a template string.** `"${sym}"` becomes `append(sym)` and
`String` declares no such overload, so it is *no 'append' takes these
arguments*. Adding one to the library is additive and needs no record.

**A hash keyed by symbols**, which is what a symbol is for in Ruby, waits on
`Hash<K, V>` existing at all.

## In code

`BUILTIN_SYMBOL` in `type_table.h`; `TK_SYMBOL` in the scanner's keywords and
in `TypeBuilder`; the literal's type and the arithmetic refusal in
`ExpressionTyper`; `Emitter::symbol_entry` and `Emitter::emit_symbol_table`.

Cases: `tests/emitter/cases/a_symbol_is_an_entry_in_a_table` — the table, two
spellings of one symbol, the pointer comparison and the text read back, with
the exit status as the verdict — and
`tests/type_table/cases/a_symbol_is_a_type_of_its_own`, whose golden is the
types and the two refusals. `tests/scanner/cases/keywords` pins the new word.
