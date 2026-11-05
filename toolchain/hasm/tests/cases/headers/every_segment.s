# .text can run and cannot be written, .rodata can only be read, .data and .bss can
# be written, .bss takes no room in the file, and the stack cannot run.
_start:
    ret
.section .rodata
    .byte 1
.data
    .byte 2
.bss
    .zero 100
