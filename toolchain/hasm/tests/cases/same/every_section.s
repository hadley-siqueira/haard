# All four, addresses through each other, in and out of order.
.section .rodata
message:
    .ascii "a\tb\n\\\"c"
    .asciz "x"
    .byte 1, 2, 255, -1
    .balign 8
    .quad 0x1122334455667788, -1
    .word 65535, -32768
    .long 4294967295, -2147483648
    .balign 16
table:
    .quad first
    .quad "second one"
    .quad counter
.data
counter:
    .long 7
    .balign 8
pointer:
    .quad message
    .zero 13
after_zero:
    .byte 9
.bss
reserved:
    .zero 100
    .balign 64
aligned:
    .zero 5000
.text
_start:
first:
    lea rax, [rip + message]
    mov eax, dword ptr [rip + counter]
    mov qword ptr [rip + reserved], 1000
    mov byte ptr [rip + aligned], 1
"second one":
    call first
    ret
.data
more_data:
    .byte 5
.text
    lea rax, [rip + more_data]
    ret
