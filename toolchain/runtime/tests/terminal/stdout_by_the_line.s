# On a terminal stdout goes at each line, so this order is the program's. On a pipe it
# would be stderr's line first.
main:
    lea rdi, [rip + one]
    call print_out
    lea rdi, [rip + two]
    call print_err
    lea rdi, [rip + three]
    call print_out
    xor eax, eax
    ret
.section .rodata
one:
    .asciz "one\n"
two:
    .asciz "two\n"
three:
    .asciz "three\n"
