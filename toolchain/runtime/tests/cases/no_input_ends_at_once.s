# A file ends only for a read that met it: not before the first one, and for good after.
main:
    push rbp
    mov rbp, rsp
    sub rsp, 8
    call __io_stdin
    mov qword ptr [rbp - 8], rax
    mov rdi, rax
    call __io_at_end
    test al, al
    jne .Lwrong
    mov rdi, qword ptr [rbp - 8]
    call __io_read
    mov rdi, qword ptr [rbp - 8]
    call __io_at_end
    test al, al
    je .Lwrong
    mov rdi, qword ptr [rbp - 8]
    call __io_read
    mov rdi, qword ptr [rbp - 8]
    call __io_at_end
    test al, al
    je .Lwrong
    xor eax, eax
    leave
    ret
.Lwrong:
    mov eax, 1
    leave
    ret
