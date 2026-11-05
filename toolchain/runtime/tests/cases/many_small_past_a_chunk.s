# 200,000 blocks of 48 bytes need more than two chunks of 4 MB: the carving has to move
# to a new one. Each keeps a mark that is looked at in the end, freed in order.
#   [rbp - 8] how many, [rbp - 16] the newest; each block links to the one before
main:
    push rbp
    mov rbp, rsp
    sub rsp, 16
    mov qword ptr [rbp - 8], 0
    mov qword ptr [rbp - 16], 0
.Lmake:
    mov edi, 48
    call __alloc
    mov rcx, qword ptr [rbp - 16]
    mov qword ptr [rax], rcx
    mov rdx, qword ptr [rbp - 8]
    mov qword ptr [rax + 40], rdx
    mov qword ptr [rbp - 16], rax
    add rdx, 1
    mov qword ptr [rbp - 8], rdx
    cmp rdx, 200000
    jb .Lmake
.Lwalk:
    mov rdi, qword ptr [rbp - 16]
    test rdi, rdi
    je .Lwalk_end
    mov rdx, qword ptr [rbp - 8]
    sub rdx, 1
    mov qword ptr [rbp - 8], rdx
    cmp rdx, qword ptr [rdi + 40]
    jne .Lwrong
    mov rax, qword ptr [rdi]
    mov qword ptr [rbp - 16], rax
    call __free
    jmp .Lwalk
.Lwalk_end:
    cmp qword ptr [rbp - 8], 0
    jne .Lwrong
    xor eax, eax
    leave
    ret
.Lwrong:
    mov eax, 1
    leave
    ret
