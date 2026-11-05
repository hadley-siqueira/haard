# The order hdc depends on: what comes before main, main, what comes after, and what
# that one prints still goes out. main's result survives the call after it.
__before_main:
    lea rdi, [rip + before]
    call print_out
    ret
main:
    lea rdi, [rip + during]
    call print_out
    mov eax, 9
    ret
__after_main:
    lea rdi, [rip + after]
    call print_out
    mov eax, 77
    ret
.section .rodata
before:
    .asciz "before\n"
during:
    .asciz "main\n"
after:
    .asciz "after\n"
