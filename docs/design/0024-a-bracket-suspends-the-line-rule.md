# 0024 — A bracket suspends the line rule

Status: **decided**, 2026-09-03, by Hadley reporting that a file which should
parse did not.

| | |
|---|---|
| A statement still **lives on one line** | unchanged |
| While a `(`, `[` or `{` is **open**, the line rule is suspended | **decided** |
| A closure's **braced block** is not one of those brackets | **decided** |

## Context

```
a = {
    name: 'Hadley',
    age: 23
}
```

did not parse. The parser reads a statement with a line rule — everything after
the token that opens it has to be found before the line ends — and it applied
that rule inside the braces as well, so the `{` was followed by the end of a
line and the statement was over.

The rule is right and the language is space sensitive on purpose. What was
wrong is where it was applied.

## Decision

**Between a bracket and the token that closes it, the line rule says nothing.**
The four bracketed forms — a call's arguments, a parenthesised group or tuple,
a list or array or hash literal, and a subscript — are one expression however
many lines they are written over, and the closing bracket may open a line of
its own at any indentation.

The argument is that **the rule exists to find the end of a statement**, and
inside a bracket the end is already written down. A rule that answers a
question nobody is asking is not a rule, it is an accident. It is Python's
implicit line joining, adopted for the same reason, and this language already
had one form that worked this way: a **template string** spans lines because
the scanner sets no `newline_before` inside one, and a template over five lines
was already one line as far as the statement rule was concerned.

**A closure's braced block is not one of those brackets.** Its contents are
statements and a statement is found by its line, so the rule comes back inside
it and goes away again after:

```
f(
    |x| {
        a = 1
        + 2
    },
    3
)
```

is a call whose lines are joined, holding a block of **two** statements. With
the rule still suspended in there they would be the one sum `a = 1 + 2`.

## Consequences

- **Nothing that parsed before stops parsing.** Every file the old rule
  accepted, the new one accepts: it only ever answers `true` where the old one
  answered `false`. One of the 398 parser cases changed its expectation, and it
  is the case that existed to pin the old answer.
- **The count is kept by hand and cannot leak.** Every increment has an
  unconditional decrement on every path out, including the path where the
  closing bracket was never found — `expect` reports and the parser unwinds
  normally, so the decrement still runs. An unclosed bracket therefore does not
  swallow the rest of the file.
- **`begin_statement` zeroes it anyway**, and that is not belt and braces: it
  is what puts the rule back for each statement **inside a braced block**
  written inside brackets. Removing it makes the closure case above one sum,
  which is a case in the suite.
- **The indentation stack is untouched.** A continuation line inside a bracket
  is inside one statement, and nothing looks at indentation in the middle of a
  statement — only where one begins.
- **The printer collapses what the source spread out.** `--pretty-print` writes
  the whole expression back on one line. That is what the printer has always
  done, it round trips, and how a printed line should be broken is a formatter's
  question that nothing has asked yet.

## Rejected

**A continuation character**, `\` at the end of a line. It is a second way to
say what the bracket already says, and the bracket is the one the reader can
see without looking for it.

**Making the block rule read the brackets too**, so that a `{` could open an
indented block anywhere. That is a different language: the braces here delimit
an expression, and the indentation delimits a block, and letting the two mean
each other is the ambiguity the space sensitivity was chosen to avoid.

## In code

`Parser::on_same_line` gives back `true` while `open_brackets` is not zero.
`parse_arguments`, `parse_parenthesis_or_tuple`, `parse_list`,
`parse_array_or_hash` and the subscript in `parse_postfix_expression` raise and
lower it; `parse_body` puts the enclosing count back after a block.

Cases in `tests/parser/`: `brackets_span_lines`,
`a_closure_in_brackets_keeps_the_line_rule`,
`an_unclosed_bracket_does_not_reach_the_next_statement`, and
`let_parenthesis_closed_on_next_line`, which used to pin the opposite.
