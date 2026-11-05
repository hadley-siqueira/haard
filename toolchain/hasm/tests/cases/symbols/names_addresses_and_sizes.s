# A label's size runs to the next label in the table; .L is not in the table, and a
# local one inside a function does not end it. Two at one address: the first is 0.
_start:
    nop
.Linside:
    nop
"second(i32)":
"third, same place":
    ret
.data
object:
    .quad 1
last:
    .byte 2
.bss
reserved:
    .zero 40
