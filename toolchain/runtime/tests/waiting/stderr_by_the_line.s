# A whole line and a part of one on stderr, then a read of a file, which is not stdin
# and pushes nothing out. The read is where it waits.
main:
    lea rdi, [rip + whole]
    call print_err
    lea rdi, [rip + partial]
    call print_err
    lea rdi, [rip + gate]
    call __io_open_read
    mov rdi, rax
    call __io_read
    xor eax, eax
    ret
.section .rodata
whole:
    .asciz "line\n"
partial:
    .asciz "partial"
gate:
    .asciz "gate"
