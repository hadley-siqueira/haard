#!/usr/bin/env bash
# One program for a whole number type: its edge values, each against each, through
# every operator, each result folded into a hash that is the exit code. A wrong
# result anywhere changes it. run.sh compares it across the three paths, so nothing
# here says what the right answer is.
set -u

type=$1

case $type in
    i8)  edges=('0' '1' '2' '7' '8' '100' '127' '0 - 1' '0 - 2' '0 - 100' '0 - 127' '0 - 127 - 1') ;;
    i16) edges=('0' '1' '2' '15' '16' '255' '256' '32767' '0 - 1' '0 - 256' '0 - 32767' '0 - 32767 - 1') ;;
    i32) edges=('0' '1' '2' '31' '32' '65536' '2147483647' '0 - 1' '0 - 65536' '0 - 2147483647' '0 - 2147483647 - 1') ;;
    i64) edges=('0' '1' '2' '63' '64' '4294967296' '9223372036854775807' '0 - 1' '0 - 4294967296' '0 - 9223372036854775807' '0 - 9223372036854775807 - 1') ;;
    u8)  edges=('0' '1' '2' '7' '8' '127' '128' '200' '255') ;;
    u16) edges=('0' '1' '2' '15' '16' '255' '256' '32768' '65535') ;;
    u32) edges=('0' '1' '2' '31' '32' '65536' '2147483648' '4294967295') ;;
    u64) edges=('0' '1' '2' '63' '64' '4294967296' '9223372036854775808' '18446744073709551615') ;;
esac

cat <<HEAD
def mix : u64
    @hash : u64
    @value : u64

    return (hash ^ value) * 1099511628211

def edge : $type
    @which : i32

HEAD

i=0

for value in "${edges[@]}"; do
    printf '    if which == %d:\n        return %s\n\n' "$i" "$value"
    i=$((i + 1))
done

cat <<BODY
    return 0

def flag : u64
    @value : bool

    if value:
        return 1

    return 0

def pair : u64
    @hash : u64
    @a : $type
    @b : $type

    let h = hash

    h = mix(h, (a + b) as u64)
    h = mix(h, (a - b) as u64)
    h = mix(h, (a * b) as u64)
    h = mix(h, (a & b) as u64)
    h = mix(h, (a | b) as u64)
    h = mix(h, (a ^ b) as u64)
    h = mix(h, (a << b) as u64)
    h = mix(h, (a >> b) as u64)
    h = mix(h, (a >>> b) as u64)
    h = mix(h, flag(a == b))
    h = mix(h, flag(a != b))
    h = mix(h, flag(a < b))
    h = mix(h, flag(a <= b))
    h = mix(h, flag(a > b))
    h = mix(h, flag(a >= b))

    if b != 0:
        h = mix(h, (a / b) as u64)
        h = mix(h, (a % b) as u64)
        h = mix(h, (a // b) as u64)

    return h

def single : u64
    @hash : u64
    @a : $type

    let h = hash

    h = mix(h, (~a) as u64)
    h = mix(h, (0 - a) as u64)
    h = mix(h, (a as i8) as u64)
    h = mix(h, (a as i16) as u64)
    h = mix(h, (a as i32) as u64)
    h = mix(h, (a as i64) as u64)
    h = mix(h, (a as u8) as u64)
    h = mix(h, (a as u16) as u64)
    h = mix(h, (a as u32) as u64)
    h = mix(h, a as u64)

    return h

def main : i32
    let hash : u64 = 14695981039346656037
    let count = ${#edges[@]}
    let i = 0

    while i < count:
        hash = single(hash, edge(i))

        let j = 0

        while j < count:
            hash = pair(hash, edge(i), edge(j))
            j += 1

        i += 1

    return (hash % 251) as i32
BODY
