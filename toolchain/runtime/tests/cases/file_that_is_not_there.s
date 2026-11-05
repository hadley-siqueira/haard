# Null, and no stop: the caller has to look at it. And a directory cannot be written.
main:
    push rbp
    mov rbp, rsp
    lea rdi, [rip + missing]
    call __io_open_read
    test rax, rax
    jne .Lwrong_read
    lea rdi, [rip + directory]
    call __io_open_write
    test rax, rax
    jne .Lwrong_write
    xor eax, eax
    leave
    ret
.Lwrong_read:
    mov eax, 1
    leave
    ret
.Lwrong_write:
    mov eax, 2
    leave
    ret
.section .rodata
missing:
    .asciz "there/is/no/such/file"
directory:
    .asciz "."
