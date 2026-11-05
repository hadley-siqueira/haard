# What the cases share. Every local is on the stack, as the back end's will be, since
# a call keeps only rsp and rbp.

# rdi: the file, rsi: the text, which ends in a zero
print:
    push rbp
    mov rbp, rsp
    sub rsp, 16
    mov qword ptr [rbp - 8], rdi
    mov qword ptr [rbp - 16], rsi
.Lprint_next:
    mov rax, qword ptr [rbp - 16]
    movzx esi, byte ptr [rax]
    test esi, esi
    je .Lprint_end
    add rax, 1
    mov qword ptr [rbp - 16], rax
    mov rdi, qword ptr [rbp - 8]
    call __io_write
    jmp .Lprint_next
.Lprint_end:
    leave
    ret

# rdi: the text, to stdout
print_out:
    push rbp
    mov rbp, rsp
    sub rsp, 8
    mov qword ptr [rbp - 8], rdi
    call __io_stdout
    mov rdi, rax
    mov rsi, qword ptr [rbp - 8]
    call print
    leave
    ret

# rdi: the text, to stderr
print_err:
    push rbp
    mov rbp, rsp
    sub rsp, 8
    mov qword ptr [rbp - 8], rdi
    call __io_stderr
    mov rdi, rax
    mov rsi, qword ptr [rbp - 8]
    call print
    leave
    ret
