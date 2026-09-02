#!/usr/bin/env bash
# StringTable tests. Dependencies: g++, bash and timeout. Nothing external.
#
#   ./run.sh          run everything, one line per check
#
# There is no expected/ directory here: the checks carry their own answers,
# because what they pin is a published specification (FNV-1a's test vectors)
# and not this implementation's output. A golden would record whatever the
# code produces; these fail if the code stops matching the record.
#
# Exits 0 if everything passed, 1 otherwise.
set -u

cd "$(dirname "$0")"

root=../..
build=${TMPDIR:-/tmp}/haard-string-table-tests

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
    "$root/src/haard/string_table/string_table.cpp"
)

mkdir -p "$build"

if ! err=$(g++ -std=c++20 -I"$root/src" -o "$build/test_string_table" \
        test_string_table.cpp "${sources[@]}" 2>&1); then
    printf '%sERROR%s failed to build test_string_table\n' "$red" "$reset"
    printf '%s\n' "$err"
    exit 1
fi

echo

if timeout 5 "$build/test_string_table"; then
    echo
    printf '%severything passed%s\n' "$green" "$reset"
    exit 0
fi

echo
printf '%ssomething failed%s\n' "$red" "$reset"
exit 1
