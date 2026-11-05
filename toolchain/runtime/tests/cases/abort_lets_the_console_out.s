# What was printed goes out, and a shell reads SIGABRT as 134. A file does not: C's
# abort() leaves it, and 'out.txt' stays empty.
main:
    lea rdi, [rip + path]
    call __io_open_write
    mov rdi, rax
    lea rsi, [rip + lost]
    call print
    lea rdi, [rip + text]
    call print_out
    call __abort
    ud2
.section .rodata
path:
    .asciz "out.txt"
lost:
    .asciz "lost"
text:
    .asciz "printed\n"
