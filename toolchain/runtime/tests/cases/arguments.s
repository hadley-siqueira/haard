# The count and the table, past the program's own name. The count is the exit code.
main:
    push rbp
    mov rbp, rsp
    sub rsp, 16
    mov qword ptr [rbp - 8], rdi
    mov qword ptr [rbp - 16], rsi
    mov rax, qword ptr [rbp - 16]
    mov rdi, qword ptr [rax + 8]
    call print_out
    lea rdi, [rip + space]
    call print_out
    mov rax, qword ptr [rbp - 16]
    mov rdi, qword ptr [rax + 16]
    call print_out
    mov rax, qword ptr [rbp - 8]
    leave
    ret
.section .rodata
space:
    .asciz " "
