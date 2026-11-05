# The names hdc writes, in a label, in a call, in a branch and in a rip operand.
.text
_start:
    call "std.io.print(i32)"
    call "operator[](Array<i32>&, i32)"
    lea rax, [rip + "a b::c"]
    lea rax, [rip + "operator[](Array<i32>&, i32)"]
    jmp "a b::c"
    je ".L local, quoted"
    jne .Lbare
"std.io.print(i32)":
    ret
"operator[](Array<i32>&, i32)":
    ret
"a b::c":
    nop
".L local, quoted":
    nop
.Lbare:
    ret
