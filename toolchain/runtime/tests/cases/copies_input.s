# Every byte of the input out again, until a read meets the end. The input has a 0xFF
# in it, which is what a read gives at the end too: only __io_at_end can tell.
main:
    push rbp
    mov rbp, rsp
    sub rsp, 8
.Lcopy_next:
    call __io_stdin
    mov rdi, rax
    call __io_read
    mov byte ptr [rbp - 8], al
    call __io_stdin
    mov rdi, rax
    call __io_at_end
    test al, al
    jne .Lcopy_end
    call __io_stdout
    mov rdi, rax
    movzx esi, byte ptr [rbp - 8]
    call __io_write
    jmp .Lcopy_next
.Lcopy_end:
    xor eax, eax
    leave
    ret
