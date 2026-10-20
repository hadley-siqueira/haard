#!/usr/bin/env bash
# Lowering tests. Dependencies: g++, bash, diff and timeout. Nothing external.
#
#   ./run.sh          run everything, one line per#   ./run.sh -u       rewrite the expected/ files with the current output
#
# Two halves (record 0077, stage 4).
#
# cases/ are this suite's own projects, one construct each: the golden is the
# structured IR the lowering wrote, then what running it printed and how it
# ended. They show WHAT is generated.
#
# Then every project of tests/emitter is lowered and run in the interpreter,
# and must end the way the C++ it was compiled to did: the same output and
# the same exit status, read off the end of tests/emitter/expected/<case>.txt.
# A case the lowering cannot do yet is written in not_yet, and is counted
# apart; one on that list that lowers now is a failure until it is taken off,
# so the list never says less than is true. A wrong answer is a failure
# whether the case is on the list or not.
#
# Read the diff before committing it: -u accepts whatever it produces, bugs
# included.
#
# Exits 0 if everything passed, 1 otherwise.
set -u

cd "$(dirname "$0")"

root=../..
build=${TMPDIR:-/tmp}/haard-lowering-tests
update=0

if [ "${1:-}" = "-u" ]; then
    update=1
fi

if [ -t 1 ] && [ -z "${NO_COLOR:-}" ]; then
    green=$'\033[32m'
    red=$'\033[31m'
    dim=$'\033[2m'
    reset=$'\033[0m'
else
    green=''
    red=''
    dim=''
    reset=''
fi

sources=(
    "$root/src/haard/token/token.cpp"
    "$root/src/haard/token_stream/token_stream.cpp"
    "$root/src/haard/ast/ast.cpp"
    "$root/src/haard/ast/ast_builder.cpp"
    "$root/src/haard/ast/ast_node.cpp"
    "$root/src/haard/ast_query/ast_query.cpp"
    "$root/src/haard/module_finder/module_finder.cpp"
    "$root/src/haard/manifest/manifest.cpp"
    "$root/src/haard/manifest/package_loader.cpp"
    "$root/src/haard/sugar/sugar_lowerer.cpp"
    "$root/src/haard/sugar/for_each_lowerer.cpp"
    "$root/src/haard/sugar/switch_lowerer.cpp"
    "$root/src/haard/compilation/compilation.cpp"
    "$root/src/haard/type_table/type_collector.cpp"
    "$root/src/haard/type_table/coercion.cpp"
    "$root/src/haard/type_table/expression_typer.cpp"
    "$root/src/haard/name_resolver/overload_resolver.cpp"
    "$root/src/haard/type_table/type_builder.cpp"
    "$root/src/haard/type_table/instantiator.cpp"
    "$root/src/haard/name_resolver/name_resolver.cpp"
    "$root/src/haard/name_resolver/use_resolver.cpp"
    "$root/src/haard/statement_checker/statement_checker.cpp"
    "$root/src/haard/override_checker/override_checker.cpp"
    "$root/src/haard/symbol_table/symbol_collector.cpp"
    "$root/src/haard/symbol_table/implicit_collector.cpp"
    "$root/src/haard/source_file/source_file.cpp"
    "$root/src/haard/log/log.cpp"
    "$root/src/haard/scanner/scanner.cpp"
    "$root/src/haard/parser/parser.cpp"
    "$root/src/haard/module/module.cpp"
    "$root/src/haard/type_table/type_table.cpp"
    "$root/src/haard/resolution_table/resolution_table.cpp"
    "$root/src/haard/string_table/string_table.cpp"
    "$root/src/haard/symbol_table/symbol_table.cpp"
    "$root/src/haard/emitter/emitter.cpp"
    "$root/src/haard/ir/target.cpp"
    "$root/src/haard/ir/ir_type_table.cpp"
    "$root/src/haard/ir/ir_layout.cpp"
    "$root/src/haard/ir/ir_type_translator.cpp"
    "$root/src/haard/ir/ir_class_table.cpp"
    "$root/src/haard/ir/ir_opcode.cpp"
    "$root/src/haard/ir/ir_function.cpp"
    "$root/src/haard/ir/ir_program.cpp"
    "$root/src/haard/ir/ir_builder.cpp"
    "$root/src/haard/ir/ir_printer.cpp"
    "$root/src/haard/ir/ir_verifier.cpp"
    "$root/src/haard/ir/ir_flattener.cpp"
    "$root/src/haard/ir/ir_interpreter.cpp"
    "$root/src/haard/lowering/ir_lowerer.cpp"
    "$root/src/haard/lowering/function_lowerer.cpp"
)

mkdir -p "$build" expected

. "$root/tests/objects.sh"

# the compiler's sources, compiled once for the whole test run and shared with
# every other suite. See tests/objects.sh
if ! haard_objects "$root" "${sources[@]}"; then
    exit 1
fi

if ! err=$(g++ -std=c++20 -I"$root/src" -o "$build/lower" \
        lower.cpp "${HAARD_OBJECTS[@]}" 2>&1); then
    printf '%sERROR%s failed to build the case runner\n' "$red" "$reset"
    printf '%s\n' "$err"
    exit 1
fi

passed=0
failed=0
known=0

echo

# ---- this suite's own cases: the IR as text ----

for directory in cases/*/; do
    [ -d "$directory" ] || continue
    name=$(basename "$directory")
    golden=expected/$name.txt
    got=$(timeout 5 "$build/lower" "cases/$name" 2>&1)

    if [ $update -eq 1 ]; then
        printf '%s\n' "$got" > "$golden"
        printf '%sWROTE%s %s\n' "$green" "$reset" "$golden"
        continue
    fi

    if [ ! -f "$golden" ]; then
        printf '%sFAIL%s  %s (%s is missing; run ./run.sh -u)\n' \
            "$red" "$reset" "$name" "$golden"
        failed=$((failed + 1))
        continue
    fi

    if ! diff -u "$golden" <(printf '%s\n' "$got") > "$build/$name.diff"; then
        printf '%sFAIL%s  %s\n' "$red" "$reset" "$name"
        sed -n '1,20p' "$build/$name.diff" | sed "s/^/           ${dim}/;s/\$/${reset}/"
        failed=$((failed + 1))
        continue
    fi

    printf '%sPASS%s  %s\n' "$green" "$reset" "$name"
    passed=$((passed + 1))
done

if [ $update -eq 1 ]; then
    echo
    echo "expected/ rewritten. Review the diff with 'git diff' before committing."
    exit 0
fi

# ---- every program of tests/emitter, against the C++ path ----

echo

for directory in ../emitter/cases/*/; do
    name=$(basename "$directory")
    golden=../emitter/expected/$name.txt

    # the end of the golden, from what the program printed or from its exit
    # status; a case the C++ path itself refused has no answer to agree with
    want=$(awk '/^--- (output|exit):/ { keep = 1 } keep' "$golden")

    if [ -z "$want" ]; then
        continue
    fi

    listed=0

    if grep -qx "$name" not_yet; then
        listed=1
    fi

    got=$(timeout 10 "$build/lower" "../emitter/cases/$name" --run 2>&1)

    if printf '%s' "$got" | head -1 | grep -q '^--- not lowered:'; then
        if [ $listed -eq 1 ]; then
            known=$((known + 1))
            continue
        fi

        printf '%sFAIL%s  emitter/%s %s\n' "$red" "$reset" "$name" "$got"
        failed=$((failed + 1))
        continue
    fi

    if [ "$got" != "$want" ]; then
        printf '%sFAIL%s  emitter/%s (the IR ends otherwise than the C++)\n' \
            "$red" "$reset" "$name"
        diff -u <(printf '%s\n' "$want") <(printf '%s\n' "$got") \
            | sed -n '3,20p' | sed "s/^/           ${dim}/;s/\$/${reset}/"
        failed=$((failed + 1))
        continue
    fi

    if [ $listed -eq 1 ]; then
        printf '%sFAIL%s  emitter/%s (lowers now: take it off not_yet)\n' \
            "$red" "$reset" "$name"
        failed=$((failed + 1))
        continue
    fi

    printf '%sPASS%s  emitter/%s\n' "$green" "$reset" "$name"
    passed=$((passed + 1))
done

echo
if [ $failed -gt 0 ]; then
    printf '%s%d passed, %d failed, %d not lowered yet%s\n' "$red" "$passed" \
        "$failed" "$known" "$reset"
    exit 1
fi

printf '%s%d passed, %d not lowered yet%s\n' "$green" "$passed" "$known" \
    "$reset"
