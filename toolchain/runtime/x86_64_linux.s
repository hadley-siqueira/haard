# The runtime of a native Haard program on x86-64 Linux: what libc gave the C++ path
# (record 0084). Nothing but the kernel is below it.
#
#   _start                  calls __before_main, main, __after_main, flushes, exits
#   __alloc, __free         size classes over mmap
#   __io_*, __abort         the natives of std.low_io
#   __abort_with            a message, then __abort
#   __floor                 until the IR has its ffloor instruction
#
# Calling convention, record 0084: whole numbers and pointers in rdi, rsi, rdx, rcx,
# r8, r9; floats in xmm0-xmm7; the result in rax or xmm0; only rsp and rbp are kept.
# A file of hdc's defines 'main', '__before_main' and '__after_main'. The '.Lrt' labels
# are this file's, since every file of a program shares its labels.
#
# 'main' is given the count and the table of arguments. Its result is the exit code.

# ---- the start and the end

_start:
    xor ebp, ebp

    # a terminal gets its stdout by the line, anything else by the block, as C's
    # does. TCGETS only works on a terminal
    sub rsp, 64
    mov eax, 16
    mov edi, 1
    mov esi, 0x5401
    mov rdx, rsp
    syscall
    add rsp, 64
    test rax, rax
    jne .Lrt_not_a_terminal
    lea rax, [rip + __stdout_file]
    mov byte ptr [rax + 33], 2
.Lrt_not_a_terminal:
    call __before_main
    mov rdi, qword ptr [rsp]
    lea rsi, [rsp + 8]
    call main
    push rax
    call __after_main
    call __flush_all
    pop rdi
    mov eax, 231
    syscall
    ud2

# every file still open, then the two a program does not close
__flush_all:
    push rbp
    mov rbp, rsp
    sub rsp, 8
    mov rax, qword ptr [rip + __open_files]
    mov qword ptr [rbp - 8], rax
.Lrt_flush_all_next:
    mov rdi, qword ptr [rbp - 8]
    test rdi, rdi
    je .Lrt_flush_all_console
    mov rax, qword ptr [rdi + 40]
    mov qword ptr [rbp - 8], rax
    call __flush_file
    jmp .Lrt_flush_all_next
.Lrt_flush_all_console:
    lea rdi, [rip + __stdout_file]
    call __flush_file
    lea rdi, [rip + __stderr_file]
    call __flush_file
    leave
    ret

# ---- stopping

# What was printed goes out, and then the signal, which is what C's abort() is: a
# shell reads it as 134. Only the console: C's abort() leaves other files unflushed
__abort:
    lea rdi, [rip + __stdout_file]
    call __flush_file
    lea rdi, [rip + __stderr_file]
    call __flush_file
    mov eax, 39
    syscall
    mov rdi, rax
    mov esi, 6
    mov eax, 62
    syscall
    mov edi, 134
    mov eax, 231
    syscall
    ud2

# rdi: the message, which ends in a zero
__abort_with:
    push rbp
    mov rbp, rsp
    sub rsp, 8
    mov qword ptr [rbp - 8], rdi
    lea rdi, [rip + __stdout_file]
    call __flush_file
.Lrt_abort_with_next:
    mov rax, qword ptr [rbp - 8]
    movzx esi, byte ptr [rax]
    test esi, esi
    je .Lrt_abort_with_end
    add rax, 1
    mov qword ptr [rbp - 8], rax
    lea rdi, [rip + __stderr_file]
    call __io_write
    jmp .Lrt_abort_with_next
.Lrt_abort_with_end:
    lea rdi, [rip + __stderr_file]
    mov esi, 10
    call __io_write
    call __abort
    ud2

# ---- the allocator
#
# A block is a power of two from 32 bytes to 1 MB, with 16 bytes of header before what
# __alloc gives back, so that is aligned to 16. A freed block goes on its size's list
# and is the next one given out, so with memory already mapped both calls take a
# bounded time, which the WCET report needs. Past 1 MB a block is its own mapping.
#
#   [block + 0]   its class, the power of two; 0 when it is its own mapping
#   [block + 8]   the size of the mapping, when it is one
#   what __alloc gives back is a free block's link to the next one

# rdi: the size. rax: the block, or it stops
__alloc:
    mov rax, 0x4000000000000000
    cmp rdi, rax
    jae .Lrt_out_of_memory
    add rdi, 16
    mov rax, 0x100000
    cmp rdi, rax
    ja .Lrt_alloc_mapping

    # the smallest class that holds it
    mov ecx, 5
    mov eax, 32
.Lrt_alloc_class:
    cmp rax, rdi
    jae .Lrt_alloc_class_found
    add rax, rax
    add ecx, 1
    jmp .Lrt_alloc_class
.Lrt_alloc_class_found:
    lea rdx, [rip + __free_lists]
    mov rsi, qword ptr [rdx + rcx*8]
    test rsi, rsi
    je .Lrt_alloc_carve
    mov rdi, qword ptr [rsi]
    mov qword ptr [rdx + rcx*8], rdi
    mov rax, rsi
    ret

    # rax is the block's size, rcx its class
.Lrt_alloc_carve:
    mov rsi, qword ptr [rip + __chunk_next]
    mov rdx, qword ptr [rip + __chunk_end]
    sub rdx, rsi
    cmp rdx, rax
    jae .Lrt_alloc_from_chunk

    # What was left of the old chunk is not used again, and is less than 1 MB
    push rax
    push rcx
    mov esi, 0x400000
    call __map
    pop rcx
    mov rsi, rax
    pop rax
    lea rdx, [rsi + 0x400000]
    mov qword ptr [rip + __chunk_end], rdx
.Lrt_alloc_from_chunk:
    lea rdx, [rsi + rax]
    mov qword ptr [rip + __chunk_next], rdx
    mov qword ptr [rsi], rcx
    lea rax, [rsi + 16]
    ret

.Lrt_alloc_mapping:
    # the kernel takes it up to a whole page, for both calls
    push rdi
    mov rsi, rdi
    call __map
    pop rdi
    mov qword ptr [rax], 0
    mov qword ptr [rax + 8], rdi
    add rax, 16
    ret

.Lrt_out_of_memory:
    lea rdi, [rip + __out_of_memory]
    call __abort_with
    ud2

# rsi: the size. rax: new memory, or it stops
__map:
    xor edi, edi
    mov edx, 3
    mov r10d, 0x22
    mov r8, -1
    xor r9d, r9d
    mov eax, 9
    syscall
    mov rdx, -4096
    cmp rax, rdx
    ja .Lrt_out_of_memory
    ret

# rdi: what __alloc gave, or null, which does nothing
__free:
    test rdi, rdi
    je .Lrt_free_end
    mov rcx, qword ptr [rdi - 16]
    test rcx, rcx
    je .Lrt_free_mapping
    lea rdx, [rip + __free_lists]
    mov rax, qword ptr [rdx + rcx*8]
    mov qword ptr [rdi], rax
    mov qword ptr [rdx + rcx*8], rdi
.Lrt_free_end:
    ret
.Lrt_free_mapping:
    mov rsi, qword ptr [rdi - 8]
    sub rdi, 16
    mov eax, 11
    syscall
    ret

# ---- the files
#
#   [file + 0]    the descriptor
#   [file + 8]    its buffer
#   [file + 16]   the position in it
#   [file + 24]   how much a read found
#   [file + 32]   a read has met the end
#   [file + 33]   0 read, 1 write by the block, 2 write by the line
#   [file + 34]   1 when __alloc made it
#   [file + 40]   the next open file; [file + 48] the one before

__io_stdin:
    lea rax, [rip + __stdin_file]
    ret

__io_stdout:
    lea rax, [rip + __stdout_file]
    ret

__io_stderr:
    lea rax, [rip + __stderr_file]
    ret

# rdi: the path. rax: the file, or null
__io_open_read:
    xor esi, esi
    xor edx, edx
    mov ecx, 0
    jmp __open

# O_WRONLY | O_CREAT | O_TRUNC, 0666, and the umask does the rest, as fopen's
__io_open_write:
    mov esi, 0x241
    mov edx, 0x1B6
    mov ecx, 1

# rdi: the path, rsi: the flags, rdx: the permissions, rcx: the mode of the file
__open:
    push rbp
    mov rbp, rsp
    sub rsp, 16
    mov qword ptr [rbp - 8], rcx
    mov eax, 2
    syscall
    test rax, rax
    js .Lrt_open_failed
    mov qword ptr [rbp - 16], rax
    mov edi, 65600
    call __alloc
    mov rcx, qword ptr [rbp - 16]
    mov qword ptr [rax], rcx
    lea rcx, [rax + 64]
    mov qword ptr [rax + 8], rcx
    mov qword ptr [rax + 16], 0
    mov qword ptr [rax + 24], 0
    mov byte ptr [rax + 32], 0
    mov rcx, qword ptr [rbp - 8]
    mov byte ptr [rax + 33], cl
    mov byte ptr [rax + 34], 1

    # first in the list of what is open
    mov rcx, qword ptr [rip + __open_files]
    mov qword ptr [rax + 40], rcx
    mov qword ptr [rax + 48], 0
    test rcx, rcx
    je .Lrt_open_first
    mov qword ptr [rcx + 48], rax
.Lrt_open_first:
    mov qword ptr [rip + __open_files], rax
    leave
    ret
.Lrt_open_failed:
    xor eax, eax
    leave
    ret

# rdi: the file. al: the byte, and what it gives at the end is not to be read
__io_read:
    cmp byte ptr [rdi + 33], 0
    jne .Lrt_read_at_end
    cmp byte ptr [rdi + 32], 0
    jne .Lrt_read_at_end
    mov rax, qword ptr [rdi + 16]
    cmp rax, qword ptr [rdi + 24]
    jae .Lrt_read_refill
.Lrt_read_take:
    mov rdx, qword ptr [rdi + 8]
    movzx ecx, byte ptr [rdx + rax]
    add rax, 1
    mov qword ptr [rdi + 16], rax
    mov eax, ecx
    ret
.Lrt_read_refill:
    push rbp
    mov rbp, rsp
    sub rsp, 8
    mov qword ptr [rbp - 8], rdi

    # a program that asks the console waits on it, so what it wrote goes first: all
    # of stderr, and stdout if it is by the line. C's does the same
    lea rax, [rip + __stdin_file]
    cmp rdi, rax
    jne .Lrt_read_system_call
    lea rdi, [rip + __stderr_file]
    call __flush_file
    lea rdi, [rip + __stdout_file]
    cmp byte ptr [rdi + 33], 2
    jne .Lrt_read_system_call
    call __flush_file
.Lrt_read_system_call:
    mov rax, qword ptr [rbp - 8]
    mov edi, dword ptr [rax]
    mov rsi, qword ptr [rax + 8]
    mov edx, 65536
    xor eax, eax
    syscall

    # -4 is EINTR, which is nothing. Any other failure ends the file, as for C
    cmp rax, -4
    je .Lrt_read_system_call
    mov rdi, qword ptr [rbp - 8]
    leave
    test rax, rax
    jle .Lrt_read_met_end
    mov qword ptr [rdi + 24], rax
    xor eax, eax
    jmp .Lrt_read_take
.Lrt_read_met_end:
    mov byte ptr [rdi + 32], 1
.Lrt_read_at_end:
    mov eax, 0xFF
    ret

# rdi: the file. al: whether a read has met the end
__io_at_end:
    movzx eax, byte ptr [rdi + 32]
    ret

# rdi: the file, sil: the byte
__io_write:
    movzx eax, byte ptr [rdi + 33]
    test eax, eax
    je .Lrt_write_end
    mov rdx, qword ptr [rdi + 8]
    mov rcx, qword ptr [rdi + 16]
    mov byte ptr [rdx + rcx], sil
    add rcx, 1
    mov qword ptr [rdi + 16], rcx
    cmp rcx, 65536
    jae __flush_file
    cmp eax, 2
    jne .Lrt_write_end
    cmp sil, 10
    je __flush_file
.Lrt_write_end:
    ret

# rdi: the file
__io_flush:
    jmp __flush_file

# Its buffer goes out, whole. stderr was unbuffered to C, so what it holds was, as far
# as anyone can tell, written before anything of stdout's that goes out now
__flush_file:
    push rbp
    mov rbp, rsp
    sub rsp, 16
    mov qword ptr [rbp - 8], rdi
    lea rax, [rip + __stdout_file]
    cmp rdi, rax
    jne .Lrt_flush_this
    lea rdi, [rip + __stderr_file]
    call __flush_file
.Lrt_flush_this:
    mov qword ptr [rbp - 16], 0
.Lrt_flush_more:
    mov rax, qword ptr [rbp - 8]
    cmp byte ptr [rax + 33], 0
    je .Lrt_flush_end
    mov rdx, qword ptr [rax + 16]
    mov rcx, qword ptr [rbp - 16]
    sub rdx, rcx
    jle .Lrt_flush_done
    mov rsi, qword ptr [rax + 8]
    add rsi, rcx
    mov edi, dword ptr [rax]
    mov eax, 1
    syscall
    cmp rax, -4
    je .Lrt_flush_more

    # any other failure loses what is left, as for C: there is no one to tell
    test rax, rax
    jle .Lrt_flush_done
    add qword ptr [rbp - 16], rax
    jmp .Lrt_flush_more
.Lrt_flush_done:
    mov rax, qword ptr [rbp - 8]
    mov qword ptr [rax + 16], 0
.Lrt_flush_end:
    leave
    ret

# rdi: the file
__io_close:
    push rbp
    mov rbp, rsp
    sub rsp, 8
    mov qword ptr [rbp - 8], rdi
    call __flush_file
    mov rax, qword ptr [rbp - 8]
    mov edi, dword ptr [rax]
    mov eax, 3
    syscall
    mov rdi, qword ptr [rbp - 8]
    cmp byte ptr [rdi + 34], 0
    je .Lrt_close_end

    # out of the list of what is open
    mov rax, qword ptr [rdi + 40]
    mov rcx, qword ptr [rdi + 48]
    test rax, rax
    je .Lrt_close_no_next
    mov qword ptr [rax + 48], rcx
.Lrt_close_no_next:
    test rcx, rcx
    je .Lrt_close_was_first
    mov qword ptr [rcx + 40], rax
    jmp .Lrt_close_free
.Lrt_close_was_first:
    mov qword ptr [rip + __open_files], rax
.Lrt_close_free:
    call __free
.Lrt_close_end:
    leave
    ret

# ---- numbers

# xmm0, rounded towards minus infinity, exactly
__floor:
    roundsd xmm0, xmm0, 9
    ret

.section .rodata
__out_of_memory:
    .asciz "out of memory"

.data
__stdin_file:
    .quad 0
    .quad __stdin_buffer
    .quad 0
    .quad 0
    .byte 0, 0, 0
    .balign 8
    .quad 0, 0

__stdout_file:
    .quad 1
    .quad __stdout_buffer
    .quad 0
    .quad 0
    .byte 0, 1, 0
    .balign 8
    .quad 0, 0

__stderr_file:
    .quad 2
    .quad __stderr_buffer
    .quad 0
    .quad 0
    .byte 0, 2, 0
    .balign 8
    .quad 0, 0

.bss
__open_files:
    .zero 8
__chunk_next:
    .zero 8
__chunk_end:
    .zero 8
__free_lists:
    .zero 256
__stdin_buffer:
    .zero 65536
__stdout_buffer:
    .zero 65536
__stderr_buffer:
    .zero 65536
