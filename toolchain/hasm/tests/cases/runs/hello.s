# Writes through the system call and exits through it: nothing but the kernel.
_start:
    mov eax, 1
    mov edi, 1
    lea rsi, [rip + message]
    mov edx, 13
    syscall
    mov eax, 60
    mov edi, 7
    syscall
.section .rodata
message:
    .ascii "hello, world\n"
