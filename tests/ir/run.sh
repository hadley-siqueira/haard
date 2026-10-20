#!/usr/bin/env bash
# IR tests. Dependencies: g++, bash, diff and timeout. Nothing external.
#
#   ./run.sh          run everything, one line per case
#   ./run.sh -u       rewrite the expected/ files with the current output
#
# The cases are C++ functions in cases.cpp that build IR by hand with the
# IrBuilder (record 0077, stage 2), since nothing lowers Haard to it yet. Each
# one's output -- the program printed, then what the verifier says -- is
# compared against expected/<case>.txt. A case named 'verifier_...' breaks
# one of the verifier's rules on purpose (question 88), and its golden holds
# the message and the program with the culprit marked '>>'.
#
# Read the diff before committing it: -u accepts whatever it produces, bugs
# included.
#
# Exits 0 if everything passed, 1 otherwise.
set -u

cd "$(dirname "$0")"

root=../..
build=${TMPDIR:-/tmp}/haard-ir-tests
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
)

mkdir -p "$build" expected

. "$root/tests/objects.sh"

# the compiler's sources, compiled once for the whole test run and shared with
# every other suite. See tests/objects.sh
if ! haard_objects "$root" "${sources[@]}"; then
    exit 1
fi

if ! err=$(g++ -std=c++20 -I"$root/src" -o "$build/cases" \
        cases.cpp "${HAARD_OBJECTS[@]}" 2>&1); then
    printf '%sERROR%s failed to build the case runner\n' "$red" "$reset"
    printf '%s\n' "$err"
    exit 1
fi

mapfile -t cases < <("$build/cases" --list)
total=${#cases[@]}
passed=0
failed=0
done=0

echo

for name in "${cases[@]}"; do
    golden=expected/$name.txt
    done=$((done + 1))

    got=$(timeout 5 "$build/cases" "$name" 2>&1)

    if [ $? -eq 124 ]; then
        printf '%s[%2d/%d]%s %sFAIL%s  %s (timed out after 5s)\n' \
            "$dim" "$done" "$total" "$reset" "$red" "$reset" "$name"
        failed=$((failed + 1))
        continue
    fi

    if [ $update -eq 1 ]; then
        printf '%s\n' "$got" > "$golden"
        printf '%s[%2d/%d]%s %sWROTE%s %s\n' \
            "$dim" "$done" "$total" "$reset" "$green" "$reset" "$golden"
        continue
    fi

    if [ ! -f "$golden" ]; then
        printf '%s[%2d/%d]%s %sFAIL%s  %s (%s is missing; run ./run.sh -u)\n' \
            "$dim" "$done" "$total" "$reset" "$red" "$reset" "$name" "$golden"
        failed=$((failed + 1))
        continue
    fi

    if ! diff -u "$golden" <(printf '%s\n' "$got") > "$build/$name.diff"; then
        printf '%s[%2d/%d]%s %sFAIL%s  %s\n' \
            "$dim" "$done" "$total" "$reset" "$red" "$reset" "$name"
        sed -n '1,20p' "$build/$name.diff" | sed "s/^/           ${dim}/;s/\$/${reset}/"
        failed=$((failed + 1))
        continue
    fi

    printf '%s[%2d/%d]%s %sPASS%s  %s\n' \
        "$dim" "$done" "$total" "$reset" "$green" "$reset" "$name"
    passed=$((passed + 1))
done

if [ $update -eq 1 ]; then
    echo
    echo "expected/ rewritten. Review the diff with 'git diff' before committing."
    exit 0
fi

echo
if [ $failed -gt 0 ]; then
    printf '%s%d of %d passed, %d failed%s\n' "$red" "$passed" "$total" "$failed" "$reset"
    exit 1
fi

printf '%s%d of %d passed%s\n' "$green" "$passed" "$total" "$reset"
