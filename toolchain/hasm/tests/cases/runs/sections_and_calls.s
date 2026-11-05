# A call through a table in .rodata that adds a .data value to a .bss one, with a
# frame, and an exit code that only comes out right if every address did.
_start:
    mov dword ptr [rip + reserved], 30
    lea rax, [rip + table]
    mov rdi, qword ptr [rax + 8]
    call rdi
    mov edi, eax
    mov eax, 60
    syscall
"add values(i32)":
    push rbp
    mov rbp, rsp
    sub rsp, 16
    mov eax, dword ptr [rip + value]
    mov dword ptr [rbp - 4], eax
    mov eax, dword ptr [rip + reserved]
    add eax, dword ptr [rbp - 4]
    leave
    ret
wrong:
    ud2
.section .rodata
table:
    .quad wrong
    .quad "add values(i32)"
.data
value:
    .long 12
.bss
    .zero 4096
reserved:
    .zero 4
