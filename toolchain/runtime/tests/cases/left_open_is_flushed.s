# Two files no one closes, with one closed between them, whose memory is then given to
# someone who fills it: the list of what is open has to have let go of it. The end of
# the program writes both, the newest and the oldest, as C's does.
#   [rbp - 8] the oldest, [rbp - 16] the closed one, [rbp - 24] the newest
main:
    push rbp
    mov rbp, rsp
    sub rsp, 24
    lea rdi, [rip + oldest]
    call __io_open_write
    mov qword ptr [rbp - 8], rax
    lea rdi, [rip + closed]
    call __io_open_write
    mov qword ptr [rbp - 16], rax
    lea rdi, [rip + newest]
    call __io_open_write
    mov qword ptr [rbp - 24], rax
    mov rdi, qword ptr [rbp - 16]
    call __io_close
    mov edi, 70000
    call __alloc
    mov ecx, 0
.Lfill:
    mov byte ptr [rax + rcx], -1
    add ecx, 1
    cmp ecx, 64
    jb .Lfill
    mov rdi, qword ptr [rbp - 8]
    lea rsi, [rip + text_oldest]
    call print
    mov rdi, qword ptr [rbp - 24]
    lea rsi, [rip + text_newest]
    call print
    xor eax, eax
    leave
    ret
.section .rodata
oldest:
    .asciz "out2.txt"
closed:
    .asciz "closed.txt"
newest:
    .asciz "out.txt"
text_oldest:
    .asciz "the oldest\n"
text_newest:
    .asciz "the newest\n"
