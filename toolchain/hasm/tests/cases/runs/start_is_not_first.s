# Falling in at the top of .text instead of at _start would exit with 9, not 5.
wrong:
    mov edi, 9
    mov eax, 60
    syscall
_start:
    mov edi, 5
    mov eax, 60
    syscall
