# The largest size there is: adding the header to it must not wrap around to a small
# block, which would look like it worked.
main:
    mov rdi, -1
    call __alloc
    mov eax, 1
    ret
