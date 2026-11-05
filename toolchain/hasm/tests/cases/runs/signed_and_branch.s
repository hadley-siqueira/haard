# A loop down to zero: the condition, the backward long branch and sign extension.
_start:
    mov ecx, 10
    xor edi, edi
.Lloop:
    movsx rax, byte ptr [rip + minus_two]
    sub rdi, rax
    sub ecx, 1
    jne .Lloop
    mov eax, 60
    syscall
.section .rodata
minus_two:
    .byte -2
