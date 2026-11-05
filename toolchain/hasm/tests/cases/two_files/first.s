# Calls across the files, and leaves its .data open: the next one starts in .text.
_start:
    call helper
    mov edi, dword ptr [rip + value]
    mov eax, 60
    syscall
.data
