#!/usr/bin/env bash
# Every instruction against every shape of operand, most of them nonsense. hasm
# refuses what it does not know, and run.sh then asks 'as' about whatever it did
# not refuse. forms.sh covers what was meant to work; this covers what nobody meant,
# which is how an assembler makes garbage in silence (record 0084).
set -u

conditions=(o no b ae e ne be a s ns p np l ge le g)

mnemonics=(ret leave nop cdq cqo syscall ud2 mfence
    add or adc sbb and sub xor cmp mov test lea movzx movsx movsxd
    not neg mul div idiv imul rol ror shl shr sar push pop jmp call
    addss addsd subss subsd mulss mulsd divss divsd minss minsd maxss maxsd
    sqrtss sqrtsd ucomiss ucomisd comiss comisd cvtss2sd cvtsd2ss
    andps andpd orps orpd xorps xorpd movss movsd
    cvtsi2ss cvtsi2sd cvttss2si cvttsd2si movd movq roundss roundsd)

for c in "${conditions[@]}"; do
    mnemonics+=("j$c" "set$c" "cmov$c")
done

shapes=(rax rbp r9 eax ebp r9d ax bp r9w al cl sil r9b xmm1 xmm9
    '[rax]' '[r13 + 8]' 'byte ptr [rax]' 'word ptr [rax]' 'dword ptr [rax]'
    'qword ptr [rax]' '[rip + "a label"]' 'byte ptr [rip + "a label"]'
    'qword ptr [rip + "a label"]' 'dword ptr [rsp + r9*4]'
    0 1 -1 9 127 128 255 256 -129 65535 65536 2147483647 2147483648 4294967295
    5000000000 0xFFFFFFFFFFFFFFFF -9223372036854775808
    '"a label"' .Lnear)

echo '_start:'
echo '.Lnear:'
echo '"a label":'

for m in "${mnemonics[@]}"; do
    echo "$m"

    for a in "${shapes[@]}"; do
        echo "$m $a"

        for b in "${shapes[@]}"; do
            echo "$m $a, $b"
        done
    done
done

# only these take three, and every other one must say no
for m in imul roundss roundsd add mov movsd; do
    for a in rax r9d bp xmm1 xmm9 '[rax]' 'qword ptr [rax]' 'dword ptr [r13 + 8]'; do
        for b in rax r9d bp al xmm1 xmm9 '[rax]' 'qword ptr [rip + "a label"]' 'dword ptr [r13 + 8]' 'word ptr [rax]' 'byte ptr [rax]' 1000; do
            for c in 0 1 -1 127 128 1000 65535 65536 2147483648 rax xmm1; do
                echo "$m $a, $b, $c"
            done
        done
    done
done
