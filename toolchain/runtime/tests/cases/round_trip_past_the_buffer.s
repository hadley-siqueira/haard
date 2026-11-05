# 200,000 bytes, three buffers' worth, every value of a byte, out to a file and back:
# what writing and reading do when a buffer fills, and a file's descriptors and close.
# The block right after the file has to be left alone: a buffer that overran into it
# would still write the right bytes. And a read gives its byte with nothing above it.
#   [rbp - 8] the file, [rbp - 16] the count, [rbp - 24] the byte read,
#   [rbp - 32] the block after the file
main:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    lea rdi, [rip + path]
    call __io_open_write
    test rax, rax
    je .Lcannot_write
    mov qword ptr [rbp - 8], rax
    mov edi, 70000
    call __alloc
    mov qword ptr [rbp - 32], rax
    mov rcx, 0x5A5A5A5A5A5A5A5A
    mov qword ptr [rax], rcx
    mov qword ptr [rbp - 16], 0
.Lwrite_next:
    mov rsi, qword ptr [rbp - 16]
    cmp rsi, 200000
    jae .Lwrite_end
    add qword ptr [rbp - 16], 1
    mov rdi, qword ptr [rbp - 8]
    call __io_write
    jmp .Lwrite_next
.Lwrite_end:
    mov rax, qword ptr [rbp - 32]
    mov rcx, 0x5A5A5A5A5A5A5A5A
    cmp qword ptr [rax], rcx
    jne .Loverran
    mov rdi, qword ptr [rbp - 8]
    call __io_close
    lea rdi, [rip + path]
    call __io_open_read
    test rax, rax
    je .Lcannot_read
    mov qword ptr [rbp - 8], rax
    mov qword ptr [rbp - 16], 0
.Lread_next:
    mov rdi, qword ptr [rbp - 8]
    call __io_read
    mov esi, eax
    mov qword ptr [rbp - 24], rsi
    mov rdi, qword ptr [rbp - 8]
    call __io_at_end
    test al, al
    jne .Lread_end
    mov rax, qword ptr [rbp - 16]
    and eax, 255
    cmp rax, qword ptr [rbp - 24]
    jne .Lread_other_byte
    add qword ptr [rbp - 16], 1
    jmp .Lread_next
.Lread_end:
    cmp qword ptr [rbp - 16], 200000
    jne .Lread_other_length
    mov rdi, qword ptr [rbp - 8]
    call __io_close
    xor eax, eax
    leave
    ret
.Lcannot_write:
    mov eax, 1
    leave
    ret
.Lcannot_read:
    mov eax, 2
    leave
    ret
.Lread_other_byte:
    mov eax, 3
    leave
    ret
.Lread_other_length:
    mov eax, 4
    leave
    ret
.Loverran:
    mov eax, 5
    leave
    ret
.section .rodata
path:
    .asciz "round_trip.bin"
