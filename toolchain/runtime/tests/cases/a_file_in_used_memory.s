# A file made in a block someone filled and freed starts as a new one all the same:
# nothing of its state may be left to whatever the memory held.
main:
    push rbp
    mov rbp, rsp
    sub rsp, 8
    call dirty_a_block
    lea rdi, [rip + path]
    call __io_open_write
    mov qword ptr [rbp - 8], rax
    mov rdi, rax
    lea rsi, [rip + text]
    call print
    mov rdi, qword ptr [rbp - 8]
    call __io_close

    # and for reading, which is what looks at whether the end has been met
    call dirty_a_block
    lea rdi, [rip + path]
    call __io_open_read
    mov qword ptr [rbp - 8], rax
    mov rdi, rax
    call __io_read
    cmp eax, 119
    jne .Lwrong
    mov rdi, qword ptr [rbp - 8]
    call __io_at_end
    test al, al
    jne .Lwrong
    xor eax, eax
    leave
    ret
.Lwrong:
    mov eax, 1
    leave
    ret

# a block the size a file takes, all of its first bytes set, and freed
dirty_a_block:
    mov edi, 70000
    call __alloc
    mov rdi, rax
    mov ecx, 0
.Lfill:
    mov byte ptr [rax + rcx], -1
    add ecx, 1
    cmp ecx, 64
    jb .Lfill
    call __free
    ret
.section .rodata
path:
    .asciz "out.txt"
text:
    .asciz "whole\n"
