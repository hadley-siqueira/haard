# Every class and past them: each block is aligned to 16 and holds all it was asked for
# while the others are alive, a freed one is given out again, and a block past a chunk
# or a class is its own mapping. Sizes 0, 1, 2, 4 ... 8 MB, doubled.
#   [rbp - 8] the size, [rbp - 16] how many, [rbp - 24] the block being looked at
main:
    push rbp
    mov rbp, rsp
    sub rsp, 240
    mov qword ptr [rbp - 8], 0
    mov qword ptr [rbp - 16], 0
.Lmake_next:
    mov rdi, qword ptr [rbp - 8]
    call __alloc
    test al, 15
    jne .Lnot_aligned
    mov rcx, qword ptr [rbp - 16]
    mov qword ptr [rbp + rcx*8 - 240], rax

    # fill it with its own number
    mov rdx, qword ptr [rbp - 8]
.Lfill_next:
    test rdx, rdx
    je .Lfill_end
    sub rdx, 1
    mov byte ptr [rax + rdx], cl
    jmp .Lfill_next
.Lfill_end:
    add qword ptr [rbp - 16], 1
    mov rax, qword ptr [rbp - 8]
    test rax, rax
    jne .Lnot_first
    mov qword ptr [rbp - 8], 1
    jmp .Lmake_next
.Lnot_first:
    add rax, rax
    mov qword ptr [rbp - 8], rax
    cmp rax, 0x800000
    jbe .Lmake_next

    # every one still holds its number, walking back down
    mov qword ptr [rbp - 16], 24
    mov qword ptr [rbp - 8], 0x800000
.Lcheck_next:
    mov rcx, qword ptr [rbp - 16]
    mov rax, qword ptr [rbp + rcx*8 - 240]
    mov rdx, qword ptr [rbp - 8]
.Lcheck_byte:
    test rdx, rdx
    je .Lcheck_end
    sub rdx, 1
    cmp byte ptr [rax + rdx], cl
    jne .Loverlap
    jmp .Lcheck_byte
.Lcheck_end:
    mov rax, qword ptr [rbp - 8]
    sar rax, 1
    mov qword ptr [rbp - 8], rax
    mov rcx, qword ptr [rbp - 16]
    sub rcx, 1
    mov qword ptr [rbp - 16], rcx
    cmp rcx, 0
    jg .Lcheck_next

    # a freed one comes back; null is nothing; the largest go back to the kernel
    mov rdi, qword ptr [rbp - 200]
    mov qword ptr [rbp - 24], rdi
    call __free
    mov edi, 16
    call __alloc
    cmp rax, qword ptr [rbp - 24]
    jne .Lnot_reused
    xor edi, edi
    call __free
    mov rdi, qword ptr [rbp - 48]
    call __free
    xor eax, eax
    leave
    ret
.Lnot_aligned:
    mov eax, 1
    leave
    ret
.Loverlap:
    mov eax, 2
    leave
    ret
.Lnot_reused:
    mov eax, 3
    leave
    ret
