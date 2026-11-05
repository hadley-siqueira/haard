# Nothing flushes it but the end of the program.
main:
    lea rdi, [rip + text]
    call print_out
    xor eax, eax
    ret
.section .rodata
text:
    .asciz "hello, world\n"
