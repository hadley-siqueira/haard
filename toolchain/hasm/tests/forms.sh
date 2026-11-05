#!/usr/bin/env bash
# Every form hasm knows, against the registers and memory operands where its
# encodings differ, as text. run.sh gives this to hasm and to 'as' and compares
# the bytes. Nothing here says what the bytes are: that is the oracle's job.
set -u

r64=(rax rcx rdx rbx rsp rbp rsi rdi r8 r9 r10 r11 r12 r13 r14 r15)
r32=(eax ecx edx ebx esp ebp esi edi r8d r9d r10d r11d r12d r13d r14d r15d)
r16=(ax cx dx bx sp bp si di r8w r9w r10w r11w r12w r13w r14w r15w)
r8=(al cl dl bl spl bpl sil dil r8b r9b r10b r11b r12b r13b r14b r15b)
xmm=(xmm0 xmm1 xmm2 xmm3 xmm4 xmm5 xmm6 xmm7 xmm8 xmm9 xmm10 xmm11 xmm12 xmm13 xmm14 xmm15)

# where ModRM and SIB change: rsp and r12 need a SIB, rbp and r13 a displacement,
# the three widths of a displacement and their edges, and a high index
mem=('[rax]' '[rcx]' '[rsp]' '[rbp]' '[rsi]' '[r8]' '[r12]' '[r13]' '[r15]'
     '[rax + 8]' '[rbp - 8]' '[rsp + 16]' '[r12 - 16]' '[r13 + 127]' '[rax - 128]'
     '[rax + 128]' '[rbp - 129]' '[r9 + 100000]' '[rax - 2147483648]' '[rax + 2147483647]'
     '[rax + 0]' '[rbp + 0]' '[rax + rcx]' '[rax + rcx*1]' '[rax + rcx*2]' '[rbp + rcx*4]'
     '[rax + r8]' '[rcx + r8*2]' '[rax + rdi*8]'
     '[r13 + r9*8]' '[rsp + r15*8 - 200]' '[r12 + rbp + 127]' '[rsi + rdi*2 + 128]'
     '[rip + "a label"]' '[rip + .Lnear]')

imm8=(0 1 -1 127 128 -128 255 0x7F)
imm16=("${imm8[@]}" 256 -129 32767 32768 65535 -32768 0xFFFF)
imm32=("${imm16[@]}" 65536 2147483647 2147483648 4294967295 -2147483648 0xFFFFFFFF)
imm64=(0 1 -1 127 128 -128 -129 2147483647 -2147483648 0x7FFFFFFF 0xFFFFFFFFFFFFFFFF 0xFFFFFFFF80000000)

echo '_start:'
echo '.Lnear:'
echo '"a label":'

sized() {
    # the name of a size, the registers of it and the immediates it takes
    case $1 in
        1) sn=byte; regs=("${r8[@]}"); imms=("${imm8[@]}") ;;
        2) sn=word; regs=("${r16[@]}"); imms=("${imm16[@]}") ;;
        4) sn=dword; regs=("${r32[@]}"); imms=("${imm32[@]}") ;;
        8) sn=qword; regs=("${r64[@]}"); imms=("${imm64[@]}") ;;
    esac
}

for size in 1 2 4 8; do
    sized $size

    for op in add or adc sbb and sub xor cmp mov test; do
        for a in "${regs[@]}"; do
            for b in "${regs[@]}"; do
                echo "$op $a, $b"
            done

            for i in "${imms[@]}"; do
                echo "$op $a, $i"
            done
        done

        for m in "${mem[@]}"; do
            for r in "${regs[0]}" "${regs[5]}" "${regs[6]}" "${regs[12]}"; do
                echo "$op $sn ptr $m, $r"
                echo "$op $m, $r"

                [ "$op" != test ] && echo "$op $r, $sn ptr $m" && echo "$op $r, $m"
            done

            for i in "${imms[@]}"; do
                echo "$op $sn ptr $m, $i"
            done
        done
    done

    for op in not neg mul div idiv imul; do
        for a in "${regs[@]}"; do
            echo "$op $a"
        done

        for m in "${mem[@]}"; do
            echo "$op $sn ptr $m"
        done
    done

    for op in rol ror shl shr sar; do
        for a in "${regs[@]}"; do
            echo "$op $a, 1"
            echo "$op $a, 0"
            echo "$op $a, 7"
            echo "$op $a, 255"
            echo "$op $a, cl"
        done

        for m in "${mem[@]}"; do
            echo "$op $sn ptr $m, 1"
            echo "$op $sn ptr $m, 3"
            echo "$op $sn ptr $m, cl"
        done
    done

    if [ $size -ne 1 ]; then
        for a in "${regs[@]}"; do
            for b in "${regs[0]}" "${regs[5]}" "${regs[12]}"; do
                echo "imul $a, $b"
                echo "imul $b, $a"

                for c in o no b ae e ne be a s ns p np l ge le g; do
                    echo "cmov$c $a, $b"
                done

                for i in "${imms[@]}"; do
                    echo "imul $a, $b, $i"
                done
            done
        done

        for m in "${mem[@]}"; do
            echo "imul ${regs[9]}, $sn ptr $m"
            echo "imul ${regs[0]}, $m"
            echo "imul ${regs[9]}, $sn ptr $m, 5"
            echo "imul ${regs[3]}, $m, 1000"
            echo "cmovl ${regs[9]}, $m"
        done

        # the two extensions, from each smaller size that has them
        for from in 1 2; do
            [ $from -ge $size ] && continue

            if [ $from -eq 1 ]; then
                source=("${r8[@]}")
                fn=byte
            else
                source=("${r16[@]}")
                fn=word
            fi

            for op in movzx movsx; do
                for a in "${regs[@]}"; do
                    for b in "${source[@]}"; do
                        echo "$op $a, $b"
                    done
                done

                for m in "${mem[@]}"; do
                    echo "$op ${regs[0]}, $fn ptr $m"
                    echo "$op ${regs[10]}, $fn ptr $m"
                done
            done
        done
    fi
done

for size in 4 8; do
    sized $size

    for m in "${mem[@]}"; do
        echo "lea ${regs[0]}, $m"
        echo "lea ${regs[5]}, $m"
        echo "lea ${regs[13]}, $m"
    done
done

# what only exists at 64 bits
for i in 2147483648 4294967295 18446744073709551615 -9223372036854775808 0x7FFFFFFFFFFFFFFF -2147483649; do
    for a in "${r64[@]}"; do
        echo "mov $a, $i"
    done
done

for a in "${r64[@]}"; do
    echo "push $a"
    echo "pop $a"
    echo "call $a"
    echo "jmp $a"

    for b in "${r32[@]}"; do
        echo "movsxd $a, $b"
    done
done

for m in "${mem[@]}"; do
    echo "movsxd r9, dword ptr $m"
    echo "movsxd rax, $m"
    echo "call qword ptr $m"
    echo "call $m"
    echo "jmp qword ptr $m"
    echo "jmp $m"
done

for c in o no b ae e ne be a s ns p np l ge le g; do
    echo "j$c .Lnear"
    echo "j$c \"a label\""

    for a in "${r8[@]}"; do
        echo "set$c $a"
    done

    for m in "${mem[@]}"; do
        echo "set$c byte ptr $m"
        echo "set$c $m"
    done
done

echo 'jmp .Lnear'
echo 'jmp "a label"'
echo 'jmp .Lfar'
echo 'je .Lfar'
echo 'call .Lnear'
echo 'call "a label"'
echo 'call .Lfar'
echo 'lea rax, [rip + .Lfar]'
echo 'mov dword ptr [rip + .Lfar], 1000'
echo 'mov byte ptr [rip + .Lfar], 1'
echo 'cmp word ptr [rip + .Lfar], 1000'
echo 'shl qword ptr [rip + .Lfar], 3'
echo 'imul rax, qword ptr [rip + .Lfar], 1000'
echo 'roundsd xmm0, qword ptr [rip + .Lfar], 9'

# SSE, each with the size its memory operand has
sse() {
    # name, size name of its memory
    for a in "${xmm[@]}"; do
        for b in xmm0 xmm7 xmm8 xmm15; do
            echo "$1 $a, $b"
        done
    done

    for m in "${mem[@]}"; do
        echo "$1 xmm3, $2 ptr $m"
        echo "$1 xmm12, $m"
    done
}

for stem in add sub mul div min max sqrt; do
    sse ${stem}ss dword
    sse ${stem}sd qword
done

sse ucomiss dword
sse ucomisd qword
sse comiss dword
sse comisd qword
sse cvtss2sd dword
sse cvtsd2ss qword
sse movss dword
sse movsd qword

for m in "${mem[@]}"; do
    echo "movss dword ptr $m, xmm3"
    echo "movss $m, xmm12"
    echo "movsd qword ptr $m, xmm3"
    echo "movsd $m, xmm12"
    echo "roundss xmm3, dword ptr $m, 9"
    echo "roundsd xmm12, qword ptr $m, 9"
    echo "cvtsi2ss xmm3, dword ptr $m"
    echo "cvtsi2ss xmm12, qword ptr $m"
    echo "cvtsi2sd xmm3, dword ptr $m"
    echo "cvtsi2sd xmm12, qword ptr $m"
    echo "cvttss2si eax, dword ptr $m"
    echo "cvttss2si r9, $m"
    echo "cvttsd2si rax, qword ptr $m"
    echo "cvttsd2si r9d, $m"
done

for op in andps andpd orps orpd xorps xorpd; do
    for a in xmm0 xmm7 xmm8 xmm15; do
        for b in xmm0 xmm7 xmm8 xmm15; do
            echo "$op $a, $b"
        done
    done
done

for x in "${xmm[@]}"; do
    for g in rax rbp r8 r15; do
        echo "movq $x, $g"
        echo "movq $g, $x"
        echo "cvtsi2ss $x, $g"
        echo "cvtsi2sd $x, $g"
        echo "cvttss2si $g, $x"
        echo "cvttsd2si $g, $x"
    done

    for g in eax ebp r8d r15d; do
        echo "movd $x, $g"
        echo "movd $g, $x"
        echo "cvtsi2ss $x, $g"
        echo "cvtsi2sd $x, $g"
        echo "cvttss2si $g, $x"
        echo "cvttsd2si $g, $x"
    done

    echo "roundss $x, xmm1, 9"
    echo "roundsd xmm9, $x, 255"
done

for op in ret leave nop cdq cqo syscall ud2 mfence; do
    echo "$op"
done

echo '.Lfar:'
