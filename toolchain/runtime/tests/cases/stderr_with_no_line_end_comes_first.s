# stderr was unbuffered to C, so a part of a line written before stdout's goes first.
main:
    lea rdi, [rip + partial]
    call print_err
    lea rdi, [rip + whole]
    call print_out
    xor eax, eax
    ret
.section .rodata
partial:
    .asciz "partial "
whole:
    .asciz "whole\n"
