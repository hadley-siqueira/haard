# 1 GB, 200,000 times, never touched: 200 TB, past the 128 TB a program can map. Only
# giving each one back to the kernel lets this end.
#   [rbp - 8] how many
main:
    push rbp
    mov rbp, rsp
    sub rsp, 8
    mov qword ptr [rbp - 8], 0
.Lnext:
    mov edi, 0x40000000
    call __alloc
    mov rdi, rax
    call __free
    mov rax, qword ptr [rbp - 8]
    add rax, 1
    mov qword ptr [rbp - 8], rax
    cmp rax, 200000
    jb .Lnext
    xor eax, eax
    leave
    ret
