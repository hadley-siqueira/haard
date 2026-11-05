#!/usr/bin/env bash
# hasm, judged by 'as' and 'ld', and by running what it makes.
#
#   tests/run.sh
#
#   forms.sh          every form it knows, generated: the same bytes as 'as'
#   combinations.sh   every instruction with every operand: what it does not refuse
#                     has the same bytes as 'as'
#   cases/same/*.s    written by hand, the same: bytes, addresses and symbols
#   cases/refused/*.s it says no, in these words, and writes nothing
#   cases/runs/*.s    it runs, with no C, and gives this output and this exit code
#   cases/symbols/*.s readelf reads the names, addresses and sizes it wrote
#   cases/headers/*.s readelf reads the segments and their permissions
#
# 'as' and 'ld' are an oracle here and nothing else (record 0084).
#
# Exits 0 if everything passed, 1 otherwise.
set -u

cd "$(dirname "$0")/.."

tests=$(pwd)/tests
hasm=$(pwd)/build/hasm
scratch=${TMPDIR:-/tmp}/haard-hasm-tests

rm -rf "$scratch"
mkdir -p "$scratch"

if ! out=$(make -s 2>&1); then
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

# A branch always takes the long form in hasm, and 'as' takes it when told. A
# label jumped to in a case is quoted or starts with a dot, so that this cannot
# meet 'jmp rax'. _start is global only for ld to find it
for_as() {
    printf '.intel_syntax noprefix\n.globl _start\n'
    sed -E 's/^([[:space:]]*)(j[a-z]+[[:space:]]+["._])/\1{disp32} \2/' "$1"
}

section_bytes() {
    objcopy -O binary --only-section="$2" "$1" "$3" 2>/dev/null || : > "$3"
    [ -e "$3" ] || : > "$3"
}

# the address and name of each symbol, and nothing about whether it is global,
# which is not a thing a whole program has
symbols_of() {
    nm -n "$1" | sed -E 's/^([0-9a-f]+) . /\1 /' | sort
}

# the size and address of the sections there are, which also says a .bss is right
sections_of() {
    readelf -SW "$1" | sed -E 's/^ *\[ *[0-9]+\] *//' \
        | awk '$1 ~ /^\.(text|rodata|data|bss)$/ && $5 != "000000" { print $1, $3, $5 }'
}

entry_of() {
    LC_ALL=C readelf -hW "$1" | awk '/Entry/ { print $NF }'
}

# the instructions, with no file name and no label: where two labels share an
# address, objdump's choice between them is no difference in the bytes
listing() {
    objdump -M intel -d "$1" | sed -n '/^ *[0-9a-f]*:\t/p' | sed -E 's/ *<[^>]*>//'
}

# 'name' is what to call it, 'source' is the file
same_as_oracle() {
    local name=$1 source=$2 section
    local dir="$scratch/$name"
    mkdir -p "$dir"

    if ! out=$("$hasm" "$source" -o "$dir/mine" 2>&1); then
        fail "$name: hasm refused it" "$out"
        return
    fi

    for_as "$source" > "$dir/for_as.s"

    if ! out=$(as --64 "$dir/for_as.s" -o "$dir/theirs.o" 2>&1); then
        fail "$name: as refused it" "$out"
        return
    fi

    if ! out=$(ld -z noexecstack -T "$tests/layout.ld" "$dir/theirs.o" -o "$dir/theirs" 2>&1); then
        fail "$name: ld refused it" "$out"
        return
    fi

    for section in .text .rodata .data; do
        section_bytes "$dir/mine" "$section" "$dir/mine$section"
        section_bytes "$dir/theirs" "$section" "$dir/theirs$section"

        if ! cmp -s "$dir/mine$section" "$dir/theirs$section"; then
            local first
            first=$(diff <(listing "$dir/mine") <(listing "$dir/theirs") | sed -n '1,12p')
            fail "$name: $section differs from as" "$first"
            return
        fi
    done

    if [ "$(symbols_of "$dir/mine")" != "$(symbols_of "$dir/theirs")" ]; then
        fail "$name: the symbols differ from ld's" \
             "$(diff <(symbols_of "$dir/mine") <(symbols_of "$dir/theirs"))"
        return
    fi

    if [ "$(sections_of "$dir/mine")" != "$(sections_of "$dir/theirs")" ]; then
        fail "$name: the sections differ from ld's" \
             "$(diff <(sections_of "$dir/mine") <(sections_of "$dir/theirs"))"
        return
    fi

    if [ "$(entry_of "$dir/mine")" != "$(entry_of "$dir/theirs")" ]; then
        fail "$name: the entry differs from ld's" \
             "$(entry_of "$dir/mine") against $(entry_of "$dir/theirs")"
        return
    fi

    passed=$((passed + 1))
}

# ---- every form

"$tests/forms.sh" > "$scratch/forms.s"
same_as_oracle forms "$scratch/forms.s"

# ---- whatever it does not refuse, of everything

"$tests/combinations.sh" > "$scratch/all.s"
"$hasm" "$scratch/all.s" -o "$scratch/all" 2> "$scratch/all.refused"
sed -E 's/^[^:]*:([0-9]+): error:.*/\1/' "$scratch/all.refused" \
    | awk 'NR == FNR { refused[$1] = 1; next } !(FNR in refused)' - "$scratch/all.s" \
    > "$scratch/combinations.s"
same_as_oracle combinations "$scratch/combinations.s"

# ---- written by hand

for source in "$tests"/cases/same/*.s; do
    same_as_oracle "same/$(basename "$source" .s)" "$source"
done

# ---- refused, in words, with nothing written

for source in "$tests"/cases/refused/*.s; do
    name=$(basename "$source" .s)
    output="$scratch/refused-$name"

    # from its own directory, so that the diagnostic quotes a path that does not
    # depend on where the repository is
    got=$(cd "$tests/cases/refused" && "$hasm" "$name.s" -o "$output" 2>&1)
    code=$?

    if [ "$code" -ne 1 ]; then
        fail "refused/$name: exit $code, expected 1" "$got"
    elif [ -e "$output" ]; then
        fail "refused/$name: a file was written"
    elif [ "$got" != "$(cat "$tests/cases/refused/$name.expected")" ]; then
        fail "refused/$name: other words" \
             "$(diff <(printf '%s\n' "$got") "$tests/cases/refused/$name.expected")"
    else
        passed=$((passed + 1))
    fi
done

# ---- runs, with no C

for source in "$tests"/cases/runs/*.s; do
    name=$(basename "$source" .s)
    output="$scratch/runs-$name"
    base="$tests/cases/runs/$name"

    if ! out=$("$hasm" "$source" -o "$output" 2>&1); then
        fail "runs/$name: hasm refused it" "$out"
        continue
    fi

    chmod +x "$output"
    got=$("$output" 2>&1)
    code=$?

    if [ "$code" -ne "$(cat "$base.code")" ]; then
        fail "runs/$name: exit $code, expected $(cat "$base.code")" "$got"
    elif [ "$got" != "$(cat "$base.expected")" ]; then
        fail "runs/$name: other output" "$(diff <(printf '%s\n' "$got") "$base.expected")"
    else
        passed=$((passed + 1))
    fi
done

# ---- several files are one program

two="$tests/cases/two_files"

if ! out=$("$hasm" "$two/first.s" "$two/second.s" -o "$scratch/two" 2>&1); then
    fail "two_files: hasm refused it" "$out"
else
    chmod +x "$scratch/two"
    "$scratch/two"
    code=$?

    if [ "$code" -ne 5 ]; then
        fail "two_files: exit $code, expected 5"
    else
        passed=$((passed + 1))
    fi
fi

# a diagnostic names the file it is about, and a label is one across all of them
got=$(cd "$two" && "$hasm" first.s redefines.s -o "$scratch/two-bad" 2>&1)

expected="redefines.s:2: error: the label '_start' is defined twice
first.s:3: error: the label 'helper' is not defined
first.s:4: error: the label 'value' is not defined"

if [ "$got" != "$expected" ] || [ -e "$scratch/two-bad" ]; then
    fail "two_files: the second file's error" "$got"
else
    passed=$((passed + 1))
fi

# ---- the symbol table, which nothing above looks at the sizes of

for source in "$tests"/cases/symbols/*.s; do
    name=$(basename "$source" .s)
    output="$scratch/symbols-$name"

    if ! out=$("$hasm" "$source" -o "$output" 2>&1); then
        fail "symbols/$name: hasm refused it" "$out"
        continue
    fi

    # readelf complains of anything malformed on its stderr, which is part of this
    got=$(readelf -sW "$output" 2>&1 | awk 'NF >= 8 && $1 ~ /^[0-9]+:$/ && $8 != "" { $1 = ""; print }')

    if [ "$got" != "$(cat "$tests/cases/symbols/$name.expected")" ]; then
        fail "symbols/$name: other symbols" \
             "$(diff <(printf '%s\n' "$got") "$tests/cases/symbols/$name.expected")"
    else
        passed=$((passed + 1))
    fi
done

# ---- the segments: what is mapped, where, how big and with what permission

for source in "$tests"/cases/headers/*.s; do
    name=$(basename "$source" .s)
    output="$scratch/headers-$name"

    if ! out=$("$hasm" "$source" -o "$output" 2>&1); then
        fail "headers/$name: hasm refused it" "$out"
        continue
    fi

    got=$(LC_ALL=C readelf -lW "$output" 2>&1 | awk '$1 == "LOAD" || $1 == "GNU_STACK"')

    if [ "$got" != "$(cat "$tests/cases/headers/$name.expected")" ]; then
        fail "headers/$name: other segments" \
             "$(diff <(printf '%s\n' "$got") "$tests/cases/headers/$name.expected")"
    else
        passed=$((passed + 1))
    fi
done

printf 'hasm: %d passed, %d failed\n' "$passed" "$failed"

[ "$failed" -eq 0 ]
