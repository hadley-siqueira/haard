# What stdout holds, then the message on stderr with its line end, then the signal.
main:
    lea rdi, [rip + printed]
    call print_out
    lea rdi, [rip + message]
    call __abort_with
    ud2
.section .rodata
printed:
    .asciz "printed\n"
message:
    .asciz "division by zero"
