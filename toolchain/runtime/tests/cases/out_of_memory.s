# An allocation no machine can make stops with a message, rather than give null.
main:
    mov rdi, 0x7000000000000000
    call __alloc
    mov eax, 1
    ret
