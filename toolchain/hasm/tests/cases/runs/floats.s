# 1.5 + 2.25 rounded down and out as a whole number; movq from the bits in .rodata.
_start:
    movsd xmm0, qword ptr [rip + one_and_a_half]
    mov rax, qword ptr [rip + two_and_a_quarter]
    movq xmm1, rax
    addsd xmm0, xmm1
    roundsd xmm0, xmm0, 9
    cvttsd2si rdi, xmm0
    mov eax, 60
    syscall
.section .rodata
one_and_a_half:
    .quad 0x3FF8000000000000
two_and_a_quarter:
    .quad 0x4002000000000000
