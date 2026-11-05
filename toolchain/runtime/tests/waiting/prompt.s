# A part of a line on each, then a read of stdin, which is where it waits.
main:
    lea rdi, [rip + on_stderr]
    call print_err
    lea rdi, [rip + on_stdout]
    call print_out
    call __io_stdin
    mov rdi, rax
    call __io_read
    xor eax, eax
    ret
.section .rodata
on_stderr:
    .asciz "e"
on_stdout:
    .asciz "o"
