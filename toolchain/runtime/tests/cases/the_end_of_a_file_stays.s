# Once a read has met the end it stays met, as C's has since glibc 2.28, even if the
# file then grows.
#   [rbp - 8] the file written, [rbp - 16] the file read
main:
    push rbp
    mov rbp, rsp
    sub rsp, 16
    lea rdi, [rip + path]
    call __io_open_write
    mov qword ptr [rbp - 8], rax
    mov rdi, rax
    lea rsi, [rip + first]
    call print
    mov rdi, qword ptr [rbp - 8]
    call __io_flush
    lea rdi, [rip + path]
    call __io_open_read
    mov qword ptr [rbp - 16], rax
    mov rdi, rax
    call __io_read
    mov rdi, qword ptr [rbp - 16]
    call __io_read
    mov rdi, qword ptr [rbp - 16]
    call __io_at_end
    test al, al
    je .Lwrong
    mov rdi, qword ptr [rbp - 8]
    lea rsi, [rip + second]
    call print
    mov rdi, qword ptr [rbp - 8]
    call __io_flush
    mov rdi, qword ptr [rbp - 16]
    call __io_read
    cmp eax, 0xFF
    jne .Lwrong
    xor eax, eax
    leave
    ret
.Lwrong:
    mov eax, 1
    leave
    ret
.section .rodata
path:
    .asciz "out.txt"
first:
    .asciz "a"
second:
    .asciz "b"
