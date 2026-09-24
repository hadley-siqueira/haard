#!/usr/bin/env bash
# The bootstrap's scanner, judged by the C++ scanner's goldens.
#
#   tests/run.sh      build it and run every case of tests/scanner
#
# There is nothing to update here and no expected/ of its own: the answer to
# every case is already written down, in ../tests/scanner/expected, by the
# scanner this one is the twin of. A case passes when the two dumps are the
# same byte for byte -- the diagnostics first, then one line per token.
#
# It runs from tests/scanner and names each case 'cases/<name>.hd', because
# that is how the goldens were written: a diagnostic quotes the path it was
# given.
#
# Exits 0 if everything passed, 1 otherwise.
set -u

cd "$(dirname "$0")/.."

bootstrap=$(pwd)
oracle=$bootstrap/../tests/scanner

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

# the manifest and the roots table are two spellings of one build, so they
# must give the same program before either is worth testing
if ! out=$(make -s table 2>&1); then
    printf '%sFAIL%s  haard.pkg and roots.tbl build different programs\n' \
        "$red" "$reset"
    printf '%s\n' "$out" | sed -n '1,20p' | sed "s/^/      ${dim}/;s/\$/${reset}/"
    exit 1
fi

if ! out=$(make -s 2>&1); then
    printf '%sFAIL%s  the bootstrap does not build\n' "$red" "$reset"
    printf '%s\n' "$out" | sed -n '1,30p' | sed "s/^/      ${dim}/;s/\$/${reset}/"
    exit 1
fi

scan=$bootstrap/build/dump_tokens
cases=("$oracle"/cases/*.hd)
total=${#cases[@]}
passed=0
failed=0
done=0

cd "$oracle"

for case in "${cases[@]}"; do
    name=$(basename "$case" .hd)
    done=$((done + 1))

    got=$(timeout 5 "$scan" "cases/$name.hd" 2>&1)
    status=$?

    if [ $status -eq 124 ]; then
        printf '[%2d/%d] %sFAIL%s  %s (timed out after 5s)\n' \
            "$done" "$total" "$red" "$reset" "$name"
        failed=$((failed + 1))
        continue
    fi

    if ! difference=$(diff -u "expected/$name.txt" <(printf '%s\n' "$got")); then
        printf '[%2d/%d] %sFAIL%s  %s\n' "$done" "$total" "$red" "$reset" "$name"
        printf '%s\n' "$difference" | sed -n '1,20p' \
            | sed "s/^/        ${dim}/;s/\$/${reset}/"
        failed=$((failed + 1))
        continue
    fi

    printf '[%2d/%d] %sPASS%s  %s\n' "$done" "$total" "$green" "$reset" "$name"
    passed=$((passed + 1))
done

echo

if [ $failed -gt 0 ]; then
    printf '%s%d of %d passed, %d failed%s\n' "$red" "$passed" "$total" \
        "$failed" "$reset"
    exit 1
fi

printf '%s%d of %d passed%s\n' "$green" "$passed" "$total" "$reset"
