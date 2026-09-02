#!/usr/bin/env bash
# Pretty printer tests. Dependencies: g++ and bash. Nothing external.
#
#   ./run.sh
#
# One binary, one line per check. Exits 0 if everything passed, 1 otherwise.
# See tests/scanner/run.sh for the suite that covers the scanner.
set -u

cd "$(dirname "$0")"

root=../..
build=${TMPDIR:-/tmp}/haard-pretty-printer-tests

if [ -t 1 ] && [ -z "${NO_COLOR:-}" ]; then
    green=$'\033[32m'
    red=$'\033[31m'
    reset=$'\033[0m'
else
    green=''
    red=''
    reset=''
fi

sources=(
    "$root/src/haard/token/token.cpp"
    "$root/src/haard/token_stream/token_stream.cpp"
    "$root/src/haard/ast/ast.cpp"
    "$root/src/haard/ast/ast_builder.cpp"
    "$root/src/haard/ast/ast_node.cpp"
    "$root/src/haard/source_file/source_file.cpp"
    "$root/src/haard/log/log.cpp"
    "$root/src/haard/scanner/scanner.cpp"
    "$root/src/haard/module/module.cpp"
    "$root/src/haard/type_table/type_table.cpp"
    "$root/src/haard/resolution_table/resolution_table.cpp"
    "$root/src/haard/string_table/string_table.cpp"
    "$root/src/haard/symbol_table/symbol_table.cpp"
    "$root/src/haard/pretty_printer/pretty_printer.cpp"
)

mkdir -p "$build"

. "$root/tests/objects.sh"

# the compiler's sources, compiled once for the whole test run and shared with
# every other suite. See tests/objects.sh
if ! haard_objects "$root" "${sources[@]}"; then
    exit 1
fi

if ! err=$(g++ -std=c++20 -I"$root/src" -o "$build/test_pretty_printer" \
        test_pretty_printer.cpp "${HAARD_OBJECTS[@]}" 2>&1); then
    printf '%sERROR%s failed to build test_pretty_printer\n' "$red" "$reset"
    printf '%s\n' "$err"
    exit 1
fi

echo

if timeout 5 "$build/test_pretty_printer" cases/declarations.hd; then
    echo
    printf '%severything passed%s\n' "$green" "$reset"
    exit 0
fi

echo
printf '%ssomething failed%s\n' "$red" "$reset"
exit 1
