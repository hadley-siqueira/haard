# Opened for writing, a file that was longer starts empty.
#   [rbp - 8] the file
main:
    push rbp
    mov rbp, rsp
    sub rsp, 8
    lea rdi, [rip + path]
    call __io_open_write
    mov qword ptr [rbp - 8], rax
    mov rdi, rax
    lea rsi, [rip + longer]
    call print
    mov rdi, qword ptr [rbp - 8]
    call __io_close
    lea rdi, [rip + path]
    call __io_open_write
    mov qword ptr [rbp - 8], rax
    mov rdi, rax
    lea rsi, [rip + shorter]
    call print
    mov rdi, qword ptr [rbp - 8]
    call __io_close
    xor eax, eax
    leave
    ret
.section .rodata
path:
    .asciz "out.txt"
longer:
    .asciz "the longer text\n"
shorter:
    .asciz "short\n"
