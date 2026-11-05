#!/usr/bin/env bash
# The runtime, by running it: programs written by hand, assembled with it by hasm and
# run with no C.
#
#   cases/<name>.s          the program, with its 'main'
#   cases/<name>.expected   stdout and stderr together, byte for byte
#   cases/<name>.code       its exit code, as a shell reads it
#   cases/<name>.args       its arguments, if it takes any
#   cases/<name>.stdin      its input; otherwise it has none
#   cases/<name>.file       what it leaves in the file 'out.txt', if it writes one
#   cases/<name>.file2      and in 'out2.txt'
#
# The output goes to one pipe, which is what makes the order of stdout and stderr part
# of the case. A case defining __before_main has its own; the others get no_globals.s.
# 'as' and 'ld' judge the runtime's bytes too, as an oracle and nothing else.
#
# Exits 0 if everything passed, 1 otherwise.
set -u

cd "$(dirname "$0")"

tests=$(pwd)
runtime=$tests/../x86_64_linux.s
hasm=$tests/../../hasm/build/hasm
scratch=${TMPDIR:-/tmp}/haard-runtime-tests

rm -rf "$scratch"
mkdir -p "$scratch"

# A program of a case is one that may be wrong: no core, and it cannot fill the disk
# or hold the suite up, which a loop writing forever otherwise does
ulimit -c 0
ulimit -f 102400
limit=30

if ! out=$(make -s -C "$tests/../../hasm" 2>&1); then
    printf 'FAIL  hasm does not build\n%s\n' "$out"
    exit 1
fi

passed=0
failed=0

fail() {
    printf 'FAIL  %s\n' "$1"
    [ $# -ge 2 ] && printf '%s\n' "$2" | sed -n '1,20p' | sed 's/^/        /'
    failed=$((failed + 1))
}

# Builds it, in the name given; the files are the case's
build() {
    local output=$1 source=$2 globals="$tests/no_globals.s"

    grep -q '^__before_main:' "$source" && globals=

    "$hasm" "$runtime" "$tests/support.s" $globals "$source" -o "$output" && chmod +x "$output"
}

for source in "$tests"/cases/*.s; do
    name=$(basename "$source" .s)
    base="$tests/cases/$name"
    dir="$scratch/$name"
    mkdir -p "$dir"

    if ! out=$(build "$dir/program" "$source" 2>&1); then
        fail "$name: hasm refused it" "$out"
        continue
    fi

    args=()
    [ -e "$base.args" ] && read -r -a args < "$base.args"

    input=/dev/null
    [ -e "$base.stdin" ] && input=$base.stdin

    # in braces, so that bash's own word about a program that aborted is not in it
    { (cd "$dir" && timeout "$limit" ./program "${args[@]}" < "$input" > got 2>&1); code=$?; } 2>/dev/null

    if [ "$code" -ne "$(cat "$base.code")" ]; then
        fail "$name: exit $code, expected $(cat "$base.code")" "$(cat "$dir/got")"
    elif ! cmp -s "$dir/got" "$base.expected"; then
        fail "$name: other output" "$(diff "$dir/got" "$base.expected")"
    elif [ -e "$base.file" ] && ! cmp -s "$dir/out.txt" "$base.file"; then
        fail "$name: other contents in out.txt"
    elif [ -e "$base.file2" ] && ! cmp -s "$dir/out2.txt" "$base.file2"; then
        fail "$name: other contents in out2.txt"
    else
        passed=$((passed + 1))
    fi
done

# ---- on a terminal, stdout goes by the line: what no pipe can show. 'script' gives
# the program one, and writes \r\n for its \n

for_terminal="$scratch/terminal"
mkdir -p "$for_terminal"

if ! out=$(build "$for_terminal/program" "$tests/terminal/stdout_by_the_line.s" 2>&1); then
    fail "terminal: hasm refused it" "$out"
else
    got=$(timeout "$limit" script -qec "$for_terminal/program" /dev/null < /dev/null | tr -d '\r')

    if [ "$got" != "$(cat "$tests/terminal/stdout_by_the_line.expected")" ]; then
        fail "terminal: other order" "$got"
    else
        passed=$((passed + 1))
    fi
fi

# ---- what shows only while a program waits: it is held at a read, by a pipe with a
# name that nothing has written to, while its output is looked at. Letting it go ends
# it, once no one holds the pipe open for writing: which is why each program is
# started with descriptor 3 closed, or it would be waiting for itself

# whether the file holds the text, waiting up to 5 seconds for it
appears() {
    local i=0

    while [ $i -lt 500 ]; do
        grep -q "$2" "$1" 2>/dev/null && return 0
        sleep 0.01
        i=$((i + 1))
    done

    return 1
}

waiting="$scratch/waiting"
mkdir -p "$waiting"

# 1: on a pipe, a read of stdin lets stderr go first, and not stdout
dir="$waiting/pipe"
mkdir -p "$dir"
mkfifo "$dir/gate"

if ! out=$(build "$dir/program" "$tests/waiting/prompt.s" 2>&1); then
    fail "waiting/pipe: hasm refused it" "$out"
else
    exec 3<> "$dir/gate"
    timeout "$limit" "$dir/program" < "$dir/gate" > "$dir/got" 2>&1 3>&- &
    program=$!

    if ! appears "$dir/got" 'e'; then
        fail "waiting/pipe: stderr did not go before the read"
    elif grep -q 'o' "$dir/got"; then
        fail "waiting/pipe: stdout went, and on a pipe it waits"
    else
        passed=$((passed + 1))
    fi

    exec 3>&-
    wait "$program"
fi

# 2: on a terminal, stdout goes too
dir="$waiting/terminal"
mkdir -p "$dir"
mkfifo "$dir/gate"

if ! out=$(build "$dir/program" "$tests/waiting/prompt.s" 2>&1); then
    fail "waiting/terminal: hasm refused it" "$out"
else
    exec 3<> "$dir/gate"
    timeout "$limit" script -qec "$dir/program" /dev/null < "$dir/gate" > "$dir/got" 2>&1 3>&- &
    program=$!

    if ! appears "$dir/got" 'o'; then
        fail "waiting/terminal: stdout did not go before the read"
    else
        passed=$((passed + 1))
    fi

    exec 3>&-
    wait "$program"
fi

# 3: stderr goes at each line, and no sooner, with no read of stdin to push it
dir="$waiting/line"
mkdir -p "$dir"
mkfifo "$dir/gate"

if ! out=$(build "$dir/program" "$tests/waiting/stderr_by_the_line.s" 2>&1); then
    fail "waiting/line: hasm refused it" "$out"
else
    exec 3<> "$dir/gate"
    (cd "$dir" && exec timeout "$limit" ./program > got 2>&1) 3>&- &
    program=$!

    if ! appears "$dir/got" 'line'; then
        fail "waiting/line: stderr's line did not go"
    elif grep -q 'partial' "$dir/got"; then
        fail "waiting/line: part of a line went"
    else
        passed=$((passed + 1))
    fi

    exec 3>&-
    wait "$program"
fi

# ---- the bytes of all of it, against 'as' and 'ld'

oracle="$scratch/oracle"
mkdir -p "$oracle"

# one file, and each part starts in .text as it would on its own
{
    cat "$runtime"
    printf '.text\n'
    cat "$tests/support.s" "$tests/no_globals.s"
    printf 'main:\n    ret\n'
} > "$oracle/all.s"

{
    printf '.intel_syntax noprefix\n.globl _start\n'
    sed -E 's/^([[:space:]]*)(j[a-z]+[[:space:]]+["._])/\1{disp32} \2/' "$oracle/all.s"
} > "$oracle/for_as.s"

if ! out=$("$hasm" "$oracle/all.s" -o "$oracle/mine" 2>&1); then
    fail "oracle: hasm refused it" "$out"
elif ! out=$(as --64 "$oracle/for_as.s" -o "$oracle/theirs.o" 2>&1); then
    fail "oracle: as refused it" "$out"
elif ! out=$(ld -z noexecstack -T "$tests/../../hasm/tests/layout.ld" "$oracle/theirs.o" -o "$oracle/theirs" 2>&1); then
    fail "oracle: ld refused it" "$out"
else
    same=true

    for section in .text .rodata .data; do
        objcopy -O binary --only-section="$section" "$oracle/mine" "$oracle/mine$section"
        objcopy -O binary --only-section="$section" "$oracle/theirs" "$oracle/theirs$section"
        cmp -s "$oracle/mine$section" "$oracle/theirs$section" || same=false
    done

    if $same; then
        passed=$((passed + 1))
    else
        fail "oracle: its bytes differ from as"
    fi
fi

printf 'runtime: %d passed, %d failed\n' "$passed" "$failed"

[ "$failed" -eq 0 ]
