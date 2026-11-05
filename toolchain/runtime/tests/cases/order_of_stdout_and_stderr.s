# On a pipe stdout waits for its buffer and stderr does not, as for C: 'first' is held
# back past 'second', until the flush; then 'third' waits for the end of the program.
main:
    lea rdi, [rip + first]
    call print_out
    lea rdi, [rip + second]
    call print_err
    call __io_stdout
    mov rdi, rax
    call __io_flush
    lea rdi, [rip + third]
    call print_out
    lea rdi, [rip + fourth]
    call print_err
    xor eax, eax
    ret
.section .rodata
first:
    .asciz "first\n"
second:
    .asciz "second\n"
third:
    .asciz "third\n"
fourth:
    .asciz "fourth\n"
