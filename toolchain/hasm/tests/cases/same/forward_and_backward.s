# A hole before and after what it names, and one with an immediate behind it, whose
# distance is counted from the end of the instruction.
_start:
    jmp .Lforward
.Lback:
    nop
    jmp .Lback
.Lforward:
    call .Lback
    cmp qword ptr [rip + value], 1000
    mov byte ptr [rip + value], 1
    roundsd xmm0, qword ptr [rip + value], 9
    ret
.data
value:
    .quad 0
