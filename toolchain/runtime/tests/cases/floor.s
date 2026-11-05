# -1.5 goes down to -2 and 2.5 down to 2, so 2 * 10 - 2 is 18.
main:
    push rbp
    mov rbp, rsp
    sub rsp, 8
    movsd xmm0, qword ptr [rip + minus_one_and_a_half]
    call __floor
    cvttsd2si rax, xmm0
    mov qword ptr [rbp - 8], rax
    movsd xmm0, qword ptr [rip + two_and_a_half]
    call __floor
    cvttsd2si rax, xmm0
    imul rax, rax, 10
    add rax, qword ptr [rbp - 8]
    leave
    ret
.section .rodata
minus_one_and_a_half:
    .quad 0xBFF8000000000000
two_and_a_half:
    .quad 0x4004000000000000
